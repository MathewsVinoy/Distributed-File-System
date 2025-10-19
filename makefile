build: server clint

out: main.c
	gcc main.c -o out

server: sever.c metadata_server.c
	gcc sever.c -o build/ser
	gcc metadata_server.c -o build/metaser 

clint: client.c
	gcc client.c -o build/cli
