CC = gcc
CFLAGS = -Wall -Wextra -O2 -fPIC
LDFLAGS = -lm

all: prog1_lib1 prog1_lib2 prog2 lib1.so lib2.so

lib1.so: impl1.c contract.h
	$(CC) $(CFLAGS) -shared -o lib1.so impl1.c $(LDFLAGS)

lib2.so: impl2.c contract.h
	$(CC) $(CFLAGS) -shared -o lib2.so impl2.c $(LDFLAGS)

prog1_lib1: prog1.c contract.h lib1.so
	$(CC) $(CFLAGS) -o prog1_lib1 prog1.c -L. -l1 -Wl,-rpath,. $(LDFLAGS)

prog1_lib2: prog1.c contract.h lib2.so
	$(CC) $(CFLAGS) -o prog1_lib2 prog1.c -L. -l2 -Wl,-rpath,. $(LDFLAGS)

prog2: prog2.c contract.h
	$(CC) $(CFLAGS) -o prog2 prog2.c -ldl

clean:
	rm -f prog1_lib1 prog1_lib2 prog2 lib1.so lib2.so

.PHONY: all clean
