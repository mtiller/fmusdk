CFLAGS = -I../include

%.o: %.c
	$(CC) -c -fPIC $(CFLAGS) @< -o $@

%.so: %.o
	$(CC) -shared -Wl,-soname,$@ -o $@ $<

all: bouncingBall.so

clean:
	rm -f *.so *.o
	rm -f *~
