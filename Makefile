all: commons client server

commons:	./commons/commons.c 
	gcc -c ./commons/commons.c && mv ./commons.o ./bin

client:	./client/client.c
	gcc -c ./client/interface.c && mv ./interface.o ./bin
	gcc -o ./client/client ./client/client.c ./bin/commons.o ./bin/interface.o -pthread

server: ./server/server.c
	gcc -o ./server/server ./server/server.c ./bin/commons.o -pthread
