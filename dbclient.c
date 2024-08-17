#include <arpa/inet.h>
#include <assert.h>
#include <errno.h>
#include <netdb.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include "msg.h"
#include <inttypes.h>

#define BUF 256
#define MAX 128

void Usage(char *progname);

int LookupName(char *name,
                unsigned short port,
                struct sockaddr_storage *ret_addr,
                size_t *ret_addrlen);

int Connect(const struct sockaddr_storage *addr,
             const size_t addrlen,
             int *ret_fd);
/*
void copyNameTo(char* name, 
                struct msg* message);*/

int main(int argc, char **argv) {
  if (argc != 3) {
    Usage(argv[0]);
  }

  unsigned short port = 0;
  if (sscanf(argv[2], "%hu", &port) != 1) {
    Usage(argv[0]);
  }

  // Get an appropriate sockaddr structure.
  struct sockaddr_storage addr;
  size_t addrlen;
  if (!LookupName(argv[1], port, &addr, &addrlen)) {
    Usage(argv[0]);
  }

  // Connect to the remote host.
  int socket_fd;
  if (!Connect(&addr, addrlen, &socket_fd)) {
    Usage(argv[0]);
  }
  else{
  /*
  // Read something from the remote host.
  // Will only read BUF-1 characters at most.
  char readbuf[BUF];
  int res;
    res = read(socket_fd, readbuf, BUF-1);
    if (res == 0) {
      printf("socket closed prematurely \n");
      close(socket_fd);
      return EXIT_FAILURE;
    }
    if (res == -1) {
      printf("socket read failure \n");
      close(socket_fd);
      return EXIT_FAILURE;
    }
    readbuf[res] = '\0';
    printf("%s", readbuf);
    */
      
      struct msg message;
    while(1){
      int j = 0;
      while(j < 128){
        message.rd.name[j] = '\0';
        j++;
      }
    
    
      char* name = (char*) malloc((MAX)*sizeof(char));
      char* id = (char*) malloc((32)*sizeof(char));
      char* input = (char*) malloc((MAX)*sizeof(char));
      uint8_t response; 
      int res;
      uint32_t idNum;
      
      
      printf("\nEnter your choice (1 to put, 2 to get, 0 to quit): ");
      if(fgets(input,MAX,stdin) != 0) {

      int choice = atoi(input);
      
      //Checks to see if the input was in the bounds
      if(choice > 2 || choice < 0){
        printf("INPUT OUT OF BOUNDS, TRY AGAIN \n");
        continue;
      }
      
      
      
      
      
      switch (choice) {
        //PUT
        case(1): 
        {
          message.type = 1;
        
          //Gets the name of the user
          printf("\nEnter the name: ");
          
          if(fgets(name, MAX, stdin) == 0){ //ERROR CHECKING
            printf("INVALID INPUT \n");
            continue;
          }
          //Stores it in message
          char* temp = name;
          int strlength = 0;
          while(*temp!='\0'){

              strlength++;
              temp++;
          }
          temp = NULL;
          char* nametemp = name;
    
          //copy the given name into message.rd.name
          int i = 0;
          for(i =0; i< strlength-1; i++){
            message.rd.name[i] = *nametemp;
            nametemp++;
	
	
          }
          nametemp = NULL;
          
          
          
          //Gets the id of the user
          printf("\nEnter the id: ");
          
          if(fgets(id, 32, stdin) == 0){ //ERROR CHECKING
            printf("INVALID INPUT \n");
            continue;
          }
          
          //Stores it in message
          
          idNum = atoi(id);
          message.rd.id = idNum;
          
          //printf("Id Number = %d\n", message.rd.id); //FOR DEBUG
          
          
          
          
          // Write something to the remote host.
          int wres = write(socket_fd, &message, sizeof(message));
          if (wres == 0) {
             printf("socket closed prematurely \n");
             close(socket_fd);
             return EXIT_FAILURE;
          }
          if (wres == -1) {
            printf("socket write failure \n");
            close(socket_fd);
            return EXIT_FAILURE;
          }
          
          
      
          //Read response from the server

          res = read(socket_fd, &response, sizeof(response));
          if (res == 0) {
            printf("socket closed prematurely \n");
            close(socket_fd);
            return EXIT_FAILURE;
          }
          if (res == -1) {
            printf("socket read failure \n");
            close(socket_fd);
            return EXIT_FAILURE;
          }
          
          //Check if the response was SUCCESS, if true then print success, otherwise print false
          if(response == SUCCESS){
            printf("put success\n");
            continue;
          }
          
          else{
            printf("put failed\n");
            continue;
          }
        }
        
        
        
        
        
        
        
        
        
        
        //GET
        case(2):
        {
          //struct record rd;
          message.type = 2;
          
          
          printf("\nEnter the id: ");
          
          //Gets the message and puts it into .id
          if(fgets(id, 32, stdin) == 0){ //ERROR CHECKING
            printf("INVALID INPUT \n");
            continue;
          }
          
          //Convert char* to an int
          idNum = atoi(id);
          message.rd.id = idNum;
          
          //Write message to the host
          int wres = write(socket_fd, &message, sizeof(message));
          if (wres == 0) {
             printf("socket closed prematurely \n");
             close(socket_fd);
             return EXIT_FAILURE;
          }
          if (wres == -1) {
            printf("socket write failure \n");
            close(socket_fd);
            return EXIT_FAILURE;
          }
          
          
          //Read the response from the host
          res = read(socket_fd, &response, sizeof(response));
          if (res == 0) {
            printf("socket closed prematurely \n");
            close(socket_fd);
          return EXIT_FAILURE;
          }
          if (res == -1) {
            printf("socket read failure \n");
            close(socket_fd);
            return EXIT_FAILURE;
          }
        
          //If the response is SUCCESS, then read the message and print out the values
          if(response == SUCCESS){
            printf("Got a response, entered message read\n"); //debug
            struct record rec;
            
            res = read(socket_fd, &rec, sizeof(rec));
            if (res == 0) {
              printf("socket closed prematurely \n");
              close(socket_fd);
              return EXIT_FAILURE;
            }
            if (res == -1) {
              printf("socket read failure \n");
              close(socket_fd);
              return EXIT_FAILURE;
            }
            
            
            printf("name: %s\n", rec.name);
            printf("id: %" PRIu32 "\n",rec.id);
            continue;
          }
          
          
          //Otherwise print fail message and loop again
          else{
            printf("get failed\n");
            continue;
          }
        
        }
        
        
        
        
        
        
        //QUIT
        case(0):
        {
          close(socket_fd);
          return EXIT_SUCCESS;
        }
      }
      }
  }

  /*
  // Write something to the remote host.
    int wres = write(socket_fd, readbuf, res);
    if (wres == 0) {
     printf("socket closed prematurely \n");
      close(socket_fd);
      return EXIT_FAILURE;
    }
    if (wres == -1) {
      printf("socket write failure \n");
      close(socket_fd);
      return EXIT_FAILURE;
    }
  }
  
  // Clean up.
  close(socket_fd);
  return EXIT_SUCCESS;
  */
}
}

/*
//Function to copy the name from a string to the message
void 
copyNameTo(char* name, struct msg* message) {
    char* temp = name;
    int strlength = 0;
    while(*temp!='\0'){

        strlength++;
        temp++;
    }
  
    char* nametemp = name;
    
    int i = 0;
    for(i =0; i< strlength; i++){
        *message.rd.name[i] = *nametemp;
        nametemp++;
	
	
    }
    
   temp = '\0';
   *nametemp = '\0';

}*/

void 
Usage(char *progname) {
  printf("usage: %s  hostname port \n", progname);
  exit(EXIT_FAILURE);
}

int 
LookupName(char *name,
                unsigned short port,
                struct sockaddr_storage *ret_addr,
                size_t *ret_addrlen) {
  struct addrinfo hints, *results;
  int retval;

  memset(&hints, 0, sizeof(hints));
  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM;

  // Do the lookup by invoking getaddrinfo().
  if ((retval = getaddrinfo(name, NULL, &hints, &results)) != 0) {
    printf( "getaddrinfo failed: %s", gai_strerror(retval));
    return 0;
  }

  // Set the port in the first result.
  if (results->ai_family == AF_INET) {
    struct sockaddr_in *v4addr =
            (struct sockaddr_in *) (results->ai_addr);
    v4addr->sin_port = htons(port);
  } else if (results->ai_family == AF_INET6) {
    struct sockaddr_in6 *v6addr =
            (struct sockaddr_in6 *)(results->ai_addr);
    v6addr->sin6_port = htons(port);
  } else {
    printf("getaddrinfo failed to provide an IPv4 or IPv6 address \n");
    freeaddrinfo(results);
    return 0;
  }

  // Return the first result.
  assert(results != NULL);
  memcpy(ret_addr, results->ai_addr, results->ai_addrlen);
  *ret_addrlen = results->ai_addrlen;

  // Clean up.
  freeaddrinfo(results);
  return 1;
}

int 
Connect(const struct sockaddr_storage *addr,
             const size_t addrlen,
             int *ret_fd) {
  // Create the socket.
  int socket_fd = socket(addr->ss_family, SOCK_STREAM, 0);
  if (socket_fd == -1) {
    printf("socket() failed: %s", strerror(errno));
    return 0;
  }

  // Connect the socket to the remote host.
  int res = connect(socket_fd,
                    (const struct sockaddr *)(addr),
                    addrlen);
  if (res == -1) {
    printf("connect() failed: %s", strerror(errno));
    return 0;
  }

  *ret_fd = socket_fd;
  return 1;
}

