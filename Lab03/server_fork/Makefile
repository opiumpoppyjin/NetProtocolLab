objects = main.o
edit: $(objects)
	cc -o server $(objects)
main.o: main.c
	cc -c -g main.c 
clean:
	rm server $(objects)
run: $(objects)
