CC = clang

PROJECT_ROOT := .

SRC = src/main.c \
      src/controller.c \
      src/processor.c \
      src/staging.c \
      src/metrics.c \
      src/db.c \
      src/fs.c \
      src/search.c \
      src/ngram.c \
      src/doc_view.c \
      external/yyjson/src/yyjson.c \
       src/staging_finalize.c

# build objects into build/
OBJ = $(SRC:%.c=build/%.o)

OUT = build/search

CFLAGS = -O2 -Wall -Wextra \
    -I$(PROJECT_ROOT)/include \
    -I$(PROJECT_ROOT)/external/yyjson/src \
    -DDB_PATH=\"$(PROJECT_ROOT)/db/c_search.db\" \
    -DWAREHOUSE_PATH=\"$(PROJECT_ROOT)/warehouse\"

LDFLAGS = -lsqlite3

.PHONY: all run clean dirs

all: dirs $(OUT)

dirs:
	@mkdir -p build/src build/external/yyjson/src db

$(OUT): $(OBJ)
	@echo "Linking $(OUT)"
	@$(CC) $(OBJ) $(LDFLAGS) -o $(OUT)

# compile rule with mirrored directory structure
build/%.o: %.c
	@echo "Compiling $<"
	@mkdir -p $(dir $@)
	@$(CC) $(CFLAGS) -c $< -o $@

run: all
	@echo "Running $(OUT)"
	@./$(OUT)

clean:
	@echo "Deleting build artifacts and database "
	@rm -rf build db/c_search.db*