all:
	clang src/main.c -o main  -lsqlite3

clean:
	rm -f search
