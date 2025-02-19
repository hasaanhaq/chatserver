all: chat-server

chat-server: chat-server.c http-server.c
	gcc chat-server.c http-server.c -std=c11 -g -o chat-server