ifeq ($(shell getconf LONG_BIT),64) 
CFLAGS=-I. -I/usr/include/mysql -L/usr/lib64/mysql
else
CFLAGS=-I. -I/usr/include/mysql -L/usr/lib/mysql
endif

CC = gcc  

ODIR=.
MYSQL_LIB = -l mysqlclient

LIBS=-lm

DEPS = rokkod.h commands.h misc.h log.h smtp.h cloud_post.h crc16.h console.h
OBJ = rokkod.o commands.c misc.o log.c smtp.c cloud_post.o crc16.o console.o database.o

%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

rokkod: $(OBJ)
	$(CC) -o $@ $^ $(CFLAGS) $(MYSQL_LIB)

.PHONY: clean

clean:
	rm -f $(ODIR)/*.o *~ $(IDIR)/*~ rokkod

