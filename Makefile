CC=gcc
CFLAGS=-std=gnu99 -Wno-deprecated-declarations
DFLAGS=-g
LIBS=-lncursesw -lcurl -loauth -ljson-c -pthread
CTAGS=ctags -R .
OBJS=main.o twt.o hashtable.o btree.o ui.o

all: twt

tags: src/*.c src/*.h
	${CTAGS}

%.o: src/%.c
	$(CC) -c -o $@ $< $(CFLAGS) $(DFLAGS)

twt: $(OBJS)
	$(CC) $(OBJS) -o twt $(DFLAGS) $(LIBS)

clean:
	rm *.o twt
