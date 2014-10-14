CC=gcc
CFLAGS=-std=gnu99 -Wno-deprecated-declarations $(shell ncursesw5-config --cflags)
DFLAGS=-g
LIBS= $(shell ncursesw5-config --libs) -lcurl -loauth -ljson-c -pthread
CTAGS=ctags -R .
OBJS=main.o twitter.o hashtable.o btree.o ui.o stringex.o

all: twtclt

tags: src/*.c src/*.h
	${CTAGS}

%.o: src/%.c
	$(CC) -c -o $@ $< $(CFLAGS) $(DFLAGS)

twtclt: $(OBJS)
	$(CC) $(OBJS) -o twtclt $(DFLAGS) $(LIBS)

clean:
	rm *.o twtclt
