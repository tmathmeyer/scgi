PROJECT := ls2files

SRC_DFS := -D__NO_FLAGS__
binary  := build/$(PROJECT)
CFLAGS  := -pedantic

src_source_files := $(wildcard src/C/*.c)
src_object_files := $(patsubst src/C/%.c, \
	build/obj/%.o, $(src_source_files))

.PHONY: all clean

all: $(binary)

clean:
	@rm -fr build

$(binary): $(src_object_files)
	@gcc -o $(binary) $(src_object_files) -lscgi

build/obj/%.o: src/C/%.c
	@mkdir -p $(shell dirname $@)
	@gcc $(CFLAGS) -c $< -o $@ $(SRC_DFS) -lscgi
