GCC_FLAGS = -Wall
SOURCE = filter.c
LIBRARIES = pthread
TARGET = filter

all:	$(SOURCE)
	make filter

filter: $(SOURCE)
	gcc $(GCC_FLAGS) $(SOURCE) -o $(TARGET) -l$(LIBRARIES)

clean:
	rm $(TARGET)

generator:	generator.c
	gcc $(GCC_FLAGS) generator.c -o generator
