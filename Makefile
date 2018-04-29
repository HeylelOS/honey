CC=clang
CFLAGS=-g -ansi -pedantic -Wall
LDFLAGS=-lpthread -larchive
HEADERS=-I./include/
NAME=hny
SOURCES=$(wildcard src/*.c)
OBJECTS=$(patsubst src/%.c, build/obj/%.o, $(SOURCES))
ifeq ($(shell uname), Darwin)
SHAREDFLAG=-dylib -macosx_version_min 10.13
LIB=build/lib/lib$(NAME).dylib
LD=ld
else
SHAREDFLAG=-shared
LIB=build/lib/lib$(NAME).so
# We cannot use ld as a linker, on macOS, no problem
# but on linux there is a problem between dynamic and
# static code
LD=clang
endif
BUILDDIRS=build build/bin build/lib build/hny
BINSOURCE=main.c
BIN=build/bin/$(NAME)

all: $(BUILDDIRS) $(BIN)

$(BUILDDIRS):
	mkdir build
	mkdir build/obj
	mkdir build/bin
	mkdir build/lib
	mkdir build/hny

build/obj/%.o: src/%.c
	$(CC) $(CFLAGS) $(HEADERS) -fPIC -o $@ -c $<

$(OBJECTS): $(SOURCES)

$(LIB): $(OBJECTS)
	$(LD) $(SHAREDFLAG) -o $@ $^ $(LDFLAGS)

$(BIN): $(BINSOURCE) $(LIB)
	$(CC) $(CFLAGS) $(HEADERS) -o $@ $< -l$(NAME) -L./build/lib

clean:
	rm -rf build

