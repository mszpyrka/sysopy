GCC_FLAGS = -Wall
GENERAL = general.c
BARBER = barber.c
CLIENT = client.c

all:	$(CLIENT) $(BARBER) $(GENERAL)
	make client
	make barber


client: $(CLIENT) $(GENERAL)
	gcc $(GCC_FLAGS) $(CLIENT) $(GENERAL) -o client

barber: $(BARBER) $(GENERAL)
	gcc $(GCC_FLAGS) $(GENERAL) $(BARBER) -o barber

clean:
	rm client barber
