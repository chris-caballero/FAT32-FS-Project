FLAGS = -std=c99 -w

all:	exe

exe:	filesystem.o
	gcc $(FLAGS) -o FAT32System.x filesystem.o 

filesystem.o:	filesystem.c
	gcc $(FLAGS) -c filesystem.c
