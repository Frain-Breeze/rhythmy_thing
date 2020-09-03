fw3xx:
	make -f Makefile3xx
	cp -f EBOOT.PBP AVCDecoder3xx

clean:
	make -f Makefile3xx clean
	rm -f *.bak