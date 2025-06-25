all:
	gcc -L. -o user1 user1.c -lksocket
	gcc -L. -o user2 user2.c -lksocket

clean:
	rm -f user1 user2