all: fmusim

CFLAGS = -I../include
OBJS = main.o fmuinit.o fmuio.o fmusim.o fmuzip.o xml_parser.o stack.o

fmusim: $(OBJS)
	$(CC) -o fmusim $(OBJS) -ldl -lexpat
