FLAGS = -std=c99 -w

all:	exe

exe:	filesystem.o
	gcc $(FLAGS) -o project3 filesystem.o 

filesystem.o:	filesystem.c
	gcc $(FLAGS) -c filesystem.c

start:
	project3 fat32.img