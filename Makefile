CC=gcc
CFLAGS=-ggdb -Wall -Wextra -pedantic
LIBS =
ifeq ($(OS),Windows_NT)
	LIBS += -lws2_32
endif
tftp: main.o tftp.o buffer.o tools.o
	$(CC) $(CFLAGS) main.o tftp.o buffer.o tools.o -o tftp $(LIBS) 
main.o: src/main.c
	$(CC) src/main.c -c
tftp.o: src/tftp.c include/tftp.h
	$(CC) src/tftp.c -c
buffer.o: src/buffer.c include/buffer.h
	$(CC) src/buffer.c -c
tools.o: src/tools.c include/tools.h
	$(CC) src/tools.c -c
clean:
	rm *.o 
