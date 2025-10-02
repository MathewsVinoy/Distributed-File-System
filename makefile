all: out server clint

out: main.c
	gcc main.c -o out

server: sever.c
	gcc sever.c -o build/ser

clint: client.c
	gcc client.c -o build/cli
