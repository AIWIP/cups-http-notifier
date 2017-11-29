.PHONY: build

build:
	rm -rf build
	mkdir build
	gcc -o build/http `cups-config --cflags` http.c `cups-config --libs`