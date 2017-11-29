.PHONY: build install

build:
	rm -rf build
	mkdir build
	gcc -o build/http `cups-config --cflags` http.c `cups-config --libs` -lcurl

install:
	cp build/http  /usr/lib/cups/notifier/
	chown root:root /usr/lib/cups/notifier/http