CFLAGS=-I. -I/usr/include/mysql

CC = gcc  

ODIR=.
MYSQL_LIB = -l mysqlclient

LIBS=-lm

DEPS = misc.h database.h
OBJ = misc.o tcp_server.o database.o

%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

server: $(OBJ)
	echo "$(CC) -o $@ $^ $(CFLAGS) $(MYSQL_LIB)" 
	$(CC) -o $@ $^ $(CFLAGS) $(MYSQL_LIB)

.PHONY: clean

clean:
	rm -f $(ODIR)/*.o *~ $(IDIR)/*~ server

