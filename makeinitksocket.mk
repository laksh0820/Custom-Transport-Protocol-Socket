all:
	gcc -o initksocket -DDEBUG initksocket.c

clean:
	rm -f initksocket