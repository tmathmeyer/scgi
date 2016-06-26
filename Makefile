PROJECT := scgi

SRC_DFS := -D__NO_FLAGS__
binary  := build/$(PROJECT)
library := build/lib$(PROJECT).a
CFLAGS  := -pedantic

src_source_files := $(wildcard src/C/*.c)
src_object_files := $(patsubst src/C/%.c, \
	build/obj/%.o, $(src_source_files))

lib_source_files := $(wildcard src/lib/C/*.c)
lib_object_files := $(patsubst src/lib/C/%.c, \
	build/lib/%.o, $(lib_source_files))
lib_header_files := $(wildcard src/lib/H/*.h)

.PHONY: all clean

all: $(binary) library

example: install
	cd example && make

install: all
	@cp $(binary) /usr/local/bin/$(PROJECT)
	@cp $(library) /usr/lib/lib$(PROJECT).a
	@mkdir -p /usr/include/$(PROJECT)
	@cp $(lib_header_files) /usr/include/$(PROJECT)/

uninstall:
	@rm /usr/local/bin/$(PROJECT)
	@rm /usr/lib/lib$(PROJECT).a
	@rm -r /usr/include/$(PROJECT)

clean:
	@rm -fr build

library: $(lib_object_files)
	@ar -cq $(library) $(lib_object_files)

build/lib/%.o: src/lib/C/%.c
	@mkdir -p $(shell dirname $@)
	@gcc $(CFLAGS) -c $< -o $@ $(SRC_DFS)

$(binary): $(src_object_files)
	@gcc -o $(binary) $(src_object_files)

build/obj/%.o: src/C/%.c
	@mkdir -p $(shell dirname $@)
	@gcc $(CFLAGS) -c $< -o $@ $(SRC_DFS)
