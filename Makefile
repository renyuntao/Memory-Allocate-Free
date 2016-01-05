CC=gcc
CFLAGS=-pthread -std=gnu99
OBJ=MemAllocFree.o

MemAllocFree: $(OBJ)
	$(CC)  -o $@  $^ $(CFLAGS)

%.o: %.c
	$(CC) -c -o $@ $^ $(CFLAGS)

clean: 
	rm $(OBJ) MemAllocFree
