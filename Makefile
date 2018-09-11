CC=gcc
CFLAGS=-Wall -g

BINS=mythreads.o libmythreads.a


all: $(BINS)

mythreads.o: mythreads.c
	$(CC) $(CFLAGS) -c mythreads.c

libmythreads.a:  mythreads.o
	ar -cvr libmythreads.a mythreads.o

clean:
	rm $(BINS)

.FORCE:
