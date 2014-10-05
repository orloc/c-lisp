all: parser

parser: main.c 
	cc -std=c99 -Wall main.c mpc.c -lm -lreadline -g -o parser
