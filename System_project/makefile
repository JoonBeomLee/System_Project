all: client server gtk

client: client.c
	gcc -o client client.c -pthread	`pkg-config --cflags --libs gtk+-2.0` 

server: server.c
	gcc -o server server.c -pthread

gtk: GTK.c
	gcc -o gtk GTK.c `pkg-config --cflags --libs gtk+-2.0` 

clean:
	rm client server gtk

