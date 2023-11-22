.PHONY: all commons client server

all: commons client server

commons: ./commons/commons.c 
	gcc -c ./commons/commons.c && mv ./commons.o ./bin

client:	./client/client.c
	gcc -c ./client/interface.c && mv ./interface.o ./bin
	gcc -c ./client/commands.c && mv ./commands.o ./bin
	gcc -o ./client/client ./client/client.c ./bin/commons.o ./bin/interface.o ./bin/commands.o -pthread

server: ./server/server.c
	gcc -c ./server/commands.c && mv ./commands.o ./bin
	gcc -o ./server/server ./server/server.c ./bin/commons.o ./bin/commands.o -pthread
