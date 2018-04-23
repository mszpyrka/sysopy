GCC_FLAGS = -Wall
CLIENT = client.c
SERVER = server.c

all:	$(CLIENT) $(SERVER)
	make client
	make server
	

client: $(CLIENT)
	gcc $(GCC_FLAGS) $(CLIENT) -o client

server: $(SERVER)
	gcc $(GCC_FLAGS) $(SERVER) -o server

clean:
	rm client server
