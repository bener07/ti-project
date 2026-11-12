run: compile
	./div.out datasets

compile:
	gcc -o div.out -ltiff divider.c -g

.PHONY: run 
