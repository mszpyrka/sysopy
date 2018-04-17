GCC_FLAGS = -Wall

all:	master.c slave.c
	make master
	make slave

master: master.c
	gcc $(GCC_FLAGS) master.c -o master

slave: slave.c
	gcc $(GCC_FLAGS) slave.c -o slave

clean:
	rm master slave
