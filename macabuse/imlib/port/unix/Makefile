CC=g++
CFLAGS=-g
all : client server

client : shm_client.c
	g++ -g shm_client.c -o client

server : shm_server.c
	g++ -g shm_server.c -o server
