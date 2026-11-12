run: compile
	./div datasets

compile:
	gcc -o div -ltiff divider.c -g

.PHONY: run 
