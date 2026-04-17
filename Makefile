CC = clang
SRC = src/main.c
OUT = build/search
DBDIR = db

all:
	mkdir -p build
	mkdir -p db
	$(CC) $(SRC) -o $(OUT) -lsqlite3

clean:
	rm -rf build/search
	rm -rf db/c_search.db