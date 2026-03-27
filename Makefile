all:
	clang src/main.c -o search -lsqlite3

clean:
	rm -f search
