all: fmusim

CFLAGS = -I../include -g
OBJS = main.o fmuinit.o fmuio.o fmusim.o fmuzip.o xml_parser.o stack.o

all: fmusim

fmusim: $(OBJS)
	$(CC) -g -o fmusim $(OBJS) -ldl -lexpat

clean:
	rm -f $(OBJS)
	rm fmusim
	rm -rf fmuTmp*

