CC = clang

PROJECT_ROOT := .

SRC = src/main.c external/yyjson/src/yyjson.c
OUT = build/search

CFLAGS = -O2 -Wall -Wextra \
         -I$(PROJECT_ROOT)/external/yyjson/src \
         -DDB_PATH=\"$(PROJECT_ROOT)/db/c_search.db\" \
         -DWAREHOUSE_PATH=\"$(PROJECT_ROOT)/warehouse\"

LDFLAGS = -lsqlite3

all:
	mkdir -p build db
	$(CC) $(SRC) $(CFLAGS) $(LDFLAGS) -o $(OUT)

run: all
	./$(OUT)

clean:
	rm -rf build/search db/c_search.db