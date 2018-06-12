GCC_FLAGS = -Wall
HEADERS = general.h
GENERAL = general.c
CLIENT = client.c 
SERVER = server.c 

all:	$(SERVER) $(CLIENT) $(GENERAL) $(HEADERS)
	make server 
	make client

client: $(CLIENT) $(GENERAL) $(HEADERS)
	gcc $(GCC_FLAGS) $(CLIENT) $(GENERAL) -o client -lrt -pthread

server: $(SERVER) $(GENERAL) $(HEADERS)
	gcc $(GCC_FLAGS) $(SERVER) $(GENERAL) -o server -lrt -pthread

clean:
	rm server client
