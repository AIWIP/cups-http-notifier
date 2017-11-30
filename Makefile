.PHONY: build install

# TODO: Write configure script 
#
# DEPENDENCIES = libjson-c-dev libcurl4-openssl-dev

build:
	rm -rf build
	mkdir build
	gcc -o build/http `cups-config --cflags` http.c `cups-config --libs` -lcurl -ljson-c

install:
	cp build/http  /usr/lib/cups/notifier/
	chown root:root /usr/lib/cups/notifier/http