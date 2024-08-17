#include <arpa/inet.h>
#include <assert.h>
#include <errno.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include "common_threads.h"
#include "msg.h"
#include <fcntl.h> // for open(), and flags

void Usage(char *progname);
void PrintOut(int fd, struct sockaddr *addr, size_t addrlen);
void PrintReverseDNS(struct sockaddr *addr, size_t addrlen);
void PrintServerSide(int client_fd, int sock_family);

int  Listen(char *portnum, int *sock_family);
void* HandleClient(void* arg);

int 
main(int argc, char **argv) {
  // Expect the port number as a command line argument.
  if (argc != 2) {
    Usage(argv[0]);
  }

  int sock_family;
  int listen_fd = Listen(argv[1], &sock_family);
  if (listen_fd <= 0) {
    // We failed to bind/listen to a socket.  Quit with failure.
    printf("Couldn't bind to any addresses.\n");
    return EXIT_FAILURE;
  }

  // Loop forever, accepting a connection from a client and doing
  // an echo trick to it.
  while (1) {
    struct sockaddr_storage caddr;
    socklen_t caddr_len = sizeof(caddr);
     int *client_fd = malloc(sizeof(int));  //we dynamically allocate memory as each thread handles its own client connection

    if (client_fd == NULL) {
        perror("Failed to allocate memory for client_fd");
        continue;  //when we cannot allocate memory, skip this round
    }

     *client_fd = accept(listen_fd,
                           (struct sockaddr *)(&caddr),
                           &caddr_len);
    if (*client_fd < 0) {
      if ((errno == EINTR) || (errno == EAGAIN) || (errno == EWOULDBLOCK))
        continue;
      printf("Failure on accept:%s \n ", strerror(errno));
      free(client_fd);
      break;
    }
//handled one at a time, instead of handleclient, we call pthread_create, and pthread_create should be the one executing handleClient
pthread_t p1;
Pthread_create(&p1,NULL,HandleClient,client_fd);
   
  }

  // Close socket
  close(listen_fd);
  return EXIT_SUCCESS;
}

void Usage(char *progname) {
  printf("usage: %s port \n", progname);
  exit(EXIT_FAILURE);
}

void 
PrintOut(int fd, struct sockaddr *addr, size_t addrlen) {
  printf("Socket [%d] is bound to: \n", fd);
  if (addr->sa_family == AF_INET) {
    // Print out the IPV4 address and port

    char astring[INET_ADDRSTRLEN];
    struct sockaddr_in *in4 = (struct sockaddr_in *)(addr);
    inet_ntop(AF_INET, &(in4->sin_addr), astring, INET_ADDRSTRLEN);
    printf(" IPv4 address %s", astring);
    printf(" and port %d\n", ntohs(in4->sin_port));

  } else if (addr->sa_family == AF_INET6) {
    // Print out the IPV6 address and port

    char astring[INET6_ADDRSTRLEN];
    struct sockaddr_in6 *in6 = (struct sockaddr_in6 *)(addr);
    inet_ntop(AF_INET6, &(in6->sin6_addr), astring, INET6_ADDRSTRLEN);
    printf("IPv6 address %s", astring);
    printf(" and port %d\n", ntohs(in6->sin6_port));

  } else {
    printf(" ???? address and port ???? \n");
  }
}

void 
PrintReverseDNS(struct sockaddr *addr, size_t addrlen) {
  char hostname[1024];  // ought to be big enough.
  if (getnameinfo(addr, addrlen, hostname, 1024, NULL, 0, 0) != 0) {
    sprintf(hostname, "[reverse DNS failed]");
  }
  printf("DNS name: %s \n", hostname);
}

void 
PrintServerSide(int client_fd, int sock_family) {
  char hname[1024];
  hname[0] = '\0';

  printf("Server side interface is ");
  if (sock_family == AF_INET) {
    // The server is using an IPv4 address.
    struct sockaddr_in srvr;
    socklen_t srvrlen = sizeof(srvr);
    char addrbuf[INET_ADDRSTRLEN];
    getsockname(client_fd, (struct sockaddr *) &srvr, &srvrlen);
    inet_ntop(AF_INET, &srvr.sin_addr, addrbuf, INET_ADDRSTRLEN);
    printf("%s", addrbuf);
    // Get the server's dns name, or return it's IP address as
    // a substitute if the dns lookup fails.
    getnameinfo((const struct sockaddr *) &srvr,
                srvrlen, hname, 1024, NULL, 0, 0);
    printf(" [%s]\n", hname);
  } else {
    // The server is using an IPv6 address.
    struct sockaddr_in6 srvr;
    socklen_t srvrlen = sizeof(srvr);
    char addrbuf[INET6_ADDRSTRLEN];
    getsockname(client_fd, (struct sockaddr *) &srvr, &srvrlen);
    inet_ntop(AF_INET6, &srvr.sin6_addr, addrbuf, INET6_ADDRSTRLEN);
    printf("%s", addrbuf);
    // Get the server's dns name, or return it's IP address as
    // a substitute if the dns lookup fails.
    getnameinfo((const struct sockaddr *) &srvr,
                srvrlen, hname, 1024, NULL, 0, 0);
    printf(" [%s]\n", hname);
  }
}

int 
Listen(char *portnum, int *sock_family) {

  // Populate the "hints" addrinfo structure for getaddrinfo().
  // ("man addrinfo")
  struct addrinfo hints;
  memset(&hints, 0, sizeof(struct addrinfo));
  hints.ai_family = AF_INET;       // IPv6 (also handles IPv4 clients)
  hints.ai_socktype = SOCK_STREAM;  // stream
  hints.ai_flags = AI_PASSIVE;      // use wildcard "in6addr_any" address
  hints.ai_flags |= AI_V4MAPPED;    // use v4-mapped v6 if no v6 found
  hints.ai_protocol = IPPROTO_TCP;  // tcp protocol
  hints.ai_canonname = NULL;
  hints.ai_addr = NULL;
  hints.ai_next = NULL;

  // Use argv[1] as the string representation of our portnumber to
  // pass in to getaddrinfo().  getaddrinfo() returns a list of
  // address structures via the output parameter "result".
  struct addrinfo *result;
  int res = getaddrinfo(NULL, portnum, &hints, &result);

  // Did addrinfo() fail?
  if (res != 0) {
	printf( "getaddrinfo failed: %s", gai_strerror(res));
    return -1;
  }

  // Loop through the returned address structures until we are able
  // to create a socket and bind to one.  The address structures are
  // linked in a list through the "ai_next" field of result.
  int listen_fd = -1;
  struct addrinfo *rp;
  for (rp = result; rp != NULL; rp = rp->ai_next) {
    listen_fd = socket(rp->ai_family,
                       rp->ai_socktype,
                       rp->ai_protocol);
    if (listen_fd == -1) {
      // Creating this socket failed.  So, loop to the next returned
      // result and try again.
      printf("socket() failed:%s \n ", strerror(errno));
      listen_fd = -1;
      continue;
    }

    // Configure the socket; we're setting a socket "option."  In
    // particular, we set "SO_REUSEADDR", which tells the TCP stack
    // so make the port we bind to available again as soon as we
    // exit, rather than waiting for a few tens of seconds to recycle it.
    int optval = 1;
    setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR,
               &optval, sizeof(optval));

    // Try binding the socket to the address and port number returned
    // by getaddrinfo().
    if (bind(listen_fd, rp->ai_addr, rp->ai_addrlen) == 0) {
      // Bind worked!  Print out the information about what
      // we bound to.
      PrintOut(listen_fd, rp->ai_addr, rp->ai_addrlen);

      // Return to the caller the address family.
      *sock_family = rp->ai_family;
      break;
    }

    // The bind failed.  Close the socket, then loop back around and
    // try the next address/port returned by getaddrinfo().
    close(listen_fd);
    listen_fd = -1;
  }

  // Free the structure returned by getaddrinfo().
  freeaddrinfo(result);

  // If we failed to bind, return failure.
  if (listen_fd == -1)
    return listen_fd;

  // Success. Tell the OS that we want this to be a listening socket.
  if (listen(listen_fd, SOMAXCONN) != 0) {
    printf("Failed to mark socket as listening:%s \n ", strerror(errno));
    close(listen_fd);
    return -1;
  }

  // Return to the client the listening file descriptor.
  return listen_fd;
}

void* HandleClient(void* arg) {

int c_fd = *((int *) arg); //each thread has its own client_fd
/**the only use of the malloc memory, is so multiple threads can handle multiple requests
 after copying fd we no longer need this memory **/
free(arg); 
while(1) {
struct msg message; //from the client-side, this struct should already be initalized
uint8_t response; //we have to send a response back to the client
ssize_t res = read(c_fd, &message, sizeof(message)); //we read the clients message
if (res == 0) {
    printf("[The client disconnected.] \n"); //we break out of the while loop if the message sent is "0"
    break;
}
if (res == -1) { //error checking, here we dont break just continue to next round, part of original code
    if ((errno == EAGAIN) || (errno == EINTR))
        continue;
    printf("Error on client socket:%s \n ", strerror(errno));
    break;
}

if (res < sizeof(message)) { //make sure the server has read and recieved the entire message, if not this error means incomplete message recieved!!
            response = FAIL; //signal the failure as it happened on the servers end (unlike previous error with input)
            write(c_fd, &response, sizeof(response));
            continue; 
}

 //switch statement where cases are based on the type
 switch (message.type) {
       case PUT:
       {
         int db_fd = open("database.dat", O_WRONLY | O_CREAT | O_APPEND, 0666);  //we create the file if not made, when creating NEED PERMISSION!!
         //0666 is for read/write for owner, group and others. General purpose for this project so it can be tested properly.
         if (db_fd == -1) {
           perror("Failed to open database file");
           response = FAIL; //again... failure on SERVER send so we send a response back
           } 
              else {
                    if (write(db_fd, &message.rd, sizeof(struct record)) == sizeof(struct record)) { 
                    /**write returns the number of bytes written, check if it matches the number of bytes from the clients message.
                    each client message WILL match the specific struct record size, we have specific padding to allign to 256 bytes
                    this means we can assume each record the client sends will match this size, and the server should be writing this amount **/
                    response = SUCCESS;
                      } 
                    else {
                    perror("Failed to store the record");
                    response = FAIL;
                    }
             close(db_fd); //remember to close the fd when we don't need it
                }
       write(c_fd, &response, sizeof(response)); //send the response back to the client
      }
       break;
       
       case GET:
            {
                 int db_fd = open("database.dat", O_RDONLY); //keep opening and closing the file, DONT make it global!!
                    if (db_fd == -1) {
                        perror("Failed to open database file");
                        response = FAIL;
                        }
                        else {
              struct record buffer; //we have to go through the database of records sequentially, this buffer will have new data as we move across database
              //check if id is equal to the one from the message...
              int found = 0; //flag if we've found the ID
              response = FAIL; //initally set the response to fail 
              while (read(db_fd, &buffer, sizeof(struct record)) == sizeof(struct record)) { 
             //think of the buffer as a record sliding through the database, we stop reading once we go to EOF, and we've read 0.
             if (buffer.id == message.rd.id) { //for each record, check its message and see if it messages from the messages.rd.id
             response = SUCCESS;
             found = 1; //update flag
             break; //as soon as we find it, we break
           }
        }
    write(c_fd, &response, sizeof(response)); //give response back to client, we only change it if we found a success, otherwise its false
     if (found) {
      write(c_fd, &buffer, sizeof(buffer)); //IF found is not 0, only then can we send the buffer of the specific record
     }
       }
    close(db_fd); 
    }
    break;
    
    default:
       {         response = FAIL;
                write(c_fd, &response, sizeof(response)); //just in case, default case return FAIL
               
                
 }
 break;






}}
       
    //remember to close client's fd
    close(c_fd);
    return NULL;
}

