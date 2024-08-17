FLAGS = -Wall -pthread -std=gnu99
INCLUDES = .
all: dbclient dbserver
clean:
	rm -f dbclient dbserver
dbserver: dbserver.c common_threads.h msg.h
	gcc -I $(INCLUDES) -o dbserver dbserver.c $(FLAGS)
dbclient: dbclient.c msg.h
	gcc -I $(INCLUDES) -o dbclient dbclient.c $(FLAGS)
