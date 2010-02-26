all:
	(cd bouncingBall; make bouncingBall.fmu)
	(cd dq; make dq.fmu)
	(cd inc; make inc.fmu)
	(cd values; make values.fmu)
	(cd fmusim; make fmusim)

%.o: %.c
	$(CC) -c -fPIC $(CFLAGS) $< -o $@

%.so: %.o
	$(CC) -shared -Wl,-soname,$@ -o $@ $<

%.fmu: %.so
	rm -rf fmu
	mkdir fmu
	mkdir fmu/binaries
	mkdir fmu/binaries/linux32
	mkdir fmu/sources
	mkdir fmu/documentation
	cp $< fmu/binaries/linux32
	-cp *.c fmu/sources
	-cp *.h fmu/sources
	cp *.html fmu/documentation
	cp *.png fmu/documentation
	cp modelDescription.xml fmu
	cp model.png fmu
	(cd fmu; zip -r ../$@ *)

clean:
	(cd bouncingBall; make dirclean)
	(cd dq; make dirclean)
	(cd inc; make dirclean)
	(cd values; make dirclean)

dirclean:
	rm -f *.so *.o *.fmu
	rm -f *~
	rm -rf fmu
