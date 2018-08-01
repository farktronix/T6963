CC=gcc
CFLAGS= -Wall -g
LD=gcc
LDFLAGS=

all: logo grayscale

logo: logo.o bmp.o
	$(LD) $(LDFLAGS) -o logo logo.o bmp.o

logo.o: logo.c
	$(CC) $(CFLAGS) -c -o logo.o logo.c

grayscale: grayscale.o bmp.o
	$(LD) $(LDFLAGS) -o grayscale grayscale.o bmp.o

grayscale.o: grayscale.c
	$(CC) $(CFLAGS) -c -o grayscale.o grayscale.c

bmp.o: bmp.c
	$(CC) $(CFLAGS) -c -o bmp.o bmp.c

clean:
	rm -f bmp.o
	rm -f logo.o
	rm -f grayscale.o
