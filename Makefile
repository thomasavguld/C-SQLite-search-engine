CC = clang

PROJECT_ROOT := .

SRC = src/main.c src/db.c src/fs.c src/json.c src/author_cache.c external/yyjson/src/yyjson.c
OBJ = $(SRC:.c=.o)

OUT = build/search

CFLAGS = -O2 -Wall -Wextra \
	-I$(PROJECT_ROOT)/include \
	-I$(PROJECT_ROOT)/external/yyjson/src \
	-DDB_PATH=\"$(PROJECT_ROOT)/db/c_search.db\" \
	-DWAREHOUSE_PATH=\"$(PROJECT_ROOT)/warehouse\"

LDFLAGS = -lsqlite3

.PHONY: all run clean

all: $(OUT)

$(OUT): $(OBJ)
	@echo "Linking $(OUT)"
	@mkdir -p build db
	@$(CC) $(OBJ) $(LDFLAGS) -o $(OUT)

%.o: %.c
	@echo "Compiling $<"
	@$(CC) $(CFLAGS) -c $< -o $@

run: all
	@echo "Running $(OUT)"
	@./$(OUT)

clean:
	@echo "Cleaning"
	@rm -rf build $(OBJ) db/c_search.db*