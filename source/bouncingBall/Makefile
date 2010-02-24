CFLAGS = -I../include

all: bouncingBall.fmu

%.o: %.c
	$(CC) -c -fPIC $(CFLAGS) $< -o $@

%.so: %.o
	$(CC) -shared -Wl,-soname,$@ -o $@ $<

%.fmu: %.so
	rm -rf fmu
	mkdir fmu
	mkdir fmu/binaries/linux32
	mkdir fmu/sources
	mkdir fmu/documentation
	cp $@ fmu/binaries/linux32
	cp *.c *.h fmu/sources
	cp *.html fmu/documentation
	cp *.png fmu/documentation
	cp modelDescription.xml fmu
	cp model.png fmu
	(cd fmu; zip ../$< *)

clean:
	rm -f *.so *.o
	rm -f *~
	rm -rf fmu
