x11wdump: x11wdump.c
	cc -I/usr/local/include -L/usr/local/lib -lX11 -v -o x11wdump x11wdump.c

clean:
	rm x11wdump
