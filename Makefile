CC=gcc
CFLAGS=-std=gnu99 -D_XOPEN_SOURCE=700 -Wall -Wno-deprecated-declarations $(shell ncursesw5-config --cflags) -I./libmojibake
DFLAGS=-g
LIBS= $(shell ncursesw5-config --libs) -lcurl -loauth -ljson-c -pthread libmojibake/libmojibake.a
CTAGS=ctags -R .
OBJS=main.o twitter.o hashtable.o btree.o ui.o stringex.o twt_time.o utf8.o
OUTNAME=twtclt

all: ${OUTNAME}

tags: src/*.c src/*.h
	${CTAGS}

%.o: src/%.c
	$(CC) -c -o $@ $< $(CFLAGS) $(DFLAGS)

${OUTNAME}: $(OBJS)
	$(CC) $(OBJS) -o ${OUTNAME} $(DFLAGS) $(LIBS)

clean:
	rm *.o ${OUTNAME}
