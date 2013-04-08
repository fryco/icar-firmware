CFLAGS=-I. -I/usr/include/mysql

CC = gcc  

ODIR=.

LIBS=-lm

DEPS = misc.h crc16.h
OBJ = tcp_client.o misc.o crc16.o

%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

console: $(OBJ)
	$(CC) -o $@ $^ $(CFLAGS)

.PHONY: clean

clean:
	rm -f $(ODIR)/*.o *~ $(IDIR)/*~ console

