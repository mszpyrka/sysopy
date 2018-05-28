GCC_FLAGS = -Wall
GENERAL = general.c
SOURCE = main.c

all:	$(SOURCE) $(GENERAL)
	gcc $(GCC_FLAGS) $(SOURCE) $(GENERAL) -o main -lrt -pthread

clean:
	rm main
