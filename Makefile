CC=gcc
CFLAGS=-std=gnu99
DFLAGS=-g
LIBS=-lncursesw -lcurl -loauth -ljson-c
CTAGS=ctags -R .

all: twt

tags: src/*.c src/*.h
	${CTAGS}

%.o: src/%.c
	$(CC) -c -o $@ $< $(CFLAGS) $(DFLAGS)

twt: main.o twt.o
	$(CC) main.o twt.o -o twt $(DFLAGS) $(LIBS)

clean:
	rm *.o twt
