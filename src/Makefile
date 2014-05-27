all: tree-conv
	mv tree-conv ../

tree-conv: tree-conv.c question.h question.c file-utils.h file-utils.c tree.h tree.c
	gcc -g -Wall -o tree-conv tree-conv.c question.c tree.c file-utils.c

