GCC_FLAGS = -Wall

static: mylib.c libtest.c mylib.h
	gcc $(GCC_FLAGS) -c mylib.c -o mylib.o
	ar rcs libstatic.a mylib.o
	gcc $(GCC_FLAGS) -c libtest.c -o libtest.o
	gcc $(GCC_FLAGS) libtest.o -o libtest_static -L. -lstatic
	rm -f *.a *.o

shared: mylib.c libtest.c mylib.h
	gcc $(GCC_FLAGS) -fPIC -c mylib.c -o mylib.o
	gcc $(GCC_FLAGS) -c libtest.c -o libtest.o
	gcc $(GCC_FLAGS) -shared -o libshared.so mylib.o
	gcc $(GCC_FLAGS) libtest.o -o libtest_shared -L. -lshared -Wl,-rpath=`pwd`
	rm -f *.o

dll:	mylib.c libtest.c mylib.h
	gcc $(GCC_FLAGS) -fPIC -c mylib.c -o mylib.o
	gcc $(GCC_FLAGS) -shared -o libshared.so mylib.o
	gcc $(GCC_FLAGS) libtest_with_dll.c -o libtest_dll -ldl

clean:
	rm -f *.o *.a *.so libtest_static libtest_shared
