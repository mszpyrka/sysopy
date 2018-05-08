GCC_FLAGS = -Wall
GENERAL = general.c
BARBER = barber.c
CLIENT = client.c

all:	$(CLIENT) $(BARBER) $(GENERAL)
	make client
	make barber


client: $(CLIENT) $(GENERAL)
	gcc $(GCC_FLAGS) $(CLIENT) $(GENERAL) -o client -lrt -pthread

barber: $(BARBER) $(GENERAL)
	gcc $(GCC_FLAGS) $(GENERAL) $(BARBER) -o barber -lrt -pthread

clean:
	rm client barber
