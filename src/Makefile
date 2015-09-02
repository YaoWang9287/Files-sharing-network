# Specify the compiler
CC = g++

CCOPTS = -g -Wall
LIBS = -lpthread

# Make the source
all:	Host Router

common.o : common.h common.cpp 
	$(CC) $(CCOPTS) -c common.cpp

common-client.o : common-client.h common-client.cpp 
	$(CC) $(CCOPTS) -c common-client.cpp
 
Host:  Host.cpp common-client.o  
	$(CC) $(CCOPTS) common-client.o Host.cpp -o Host $(LIBS) 

Router: Router.cpp common.o
	$(CC) $(CCOPTS) common.o Router.cpp -o Router $(LIBS) 

clean :
	rm -f common.o common-client.o Host Router
