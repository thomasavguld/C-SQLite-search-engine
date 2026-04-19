CC = clang

PROJECT_ROOT := .

SRC = src/main.c src/db.c src/fs.c src/json.c external/yyjson/src/yyjson.c
OBJ = $(SRC:.c=.o)

OUT = build/search

CFLAGS = -O2 -Wall -Wextra \
	 -I$(PROJECT_ROOT)/include \
         -I$(PROJECT_ROOT)/external/yyjson/src \
         -DDB_PATH=\"$(PROJECT_ROOT)/db/c_search.db\" \
         -DWAREHOUSE_PATH=\"$(PROJECT_ROOT)/warehouse\"

LDFLAGS = -lsqlite3

all: $(OUT)

$(OUT): $(OBJ)
	mkdir -p build db
	$(CC) $(OBJ) $(LDFLAGS) -o $(OUT)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

run: all
	./$(OUT)

clean: 
	rm -rf build src/*.o external/yyjson/src/*.o db/c_search.db*