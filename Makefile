zipwriter: main.c zipwriter.c zipwriter.h
	gcc -Wall -I. -o zipwriter zipwriter.c main.c
