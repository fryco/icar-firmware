CFLAGS=-I. -I/usr/include/mysql

CC = gcc  

ODIR=.
MYSQL_LIB = -l mysqlclient

LIBS=-lm

DEPS = rokkod.h misc.h
OBJ = rokkod.o misc.o

%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

rokkod: $(OBJ)
	$(CC) -o $@ $^ $(CFLAGS) $(MYSQL_LIB)

.PHONY: clean

clean:
	rm -f $(ODIR)/*.o *~ $(IDIR)/*~ rokkod

