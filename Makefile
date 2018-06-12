all: snooplib.so
snooplib.so: src/snooplib.c
	gcc -shared -fPIC -O2 $^ -o snooplib.so -ldl
clean:
	rm snooplib.so
