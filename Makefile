CC=clang
CFLAGS= -g -ansi -pedantic -Wall
LDFLAGS= -lpthread -larchive
HEADERS=-I./include/
LIBNAME=hny
SOURCES=$(wildcard sources/*.c)
OBJECTS=$(patsubst sources/%.c, objects/%.o, $(SOURCES))
ifeq ($(shell uname), Darwin)
SHAREDFLAG=-dylib -macosx_version_min 10.13
LIB=./lib/lib$(LIBNAME).dylib
LD=ld
else
SHAREDFLAG=-shared
LIB=./lib/lib$(LIBNAME).so
# We cannot use ld as a linker, on macOS, no problem
# but on linux there is a problem between dynamic and
# static code
LD=clang
endif
BIN=./bin/$(LIBNAME)

all: $(BIN)

objects/%.o: sources/%.c
	$(CC) $(CFLAGS) $(HEADERS) -fPIC -o $@ -c $<

$(OBJECTS): $(SOURCES)

$(LIB): $(OBJECTS)
	$(LD) $(SHAREDFLAG) -o $@ $^ $(LDFLAGS)

$(BIN): $(wildcard main.c) $(LIB)
	$(CC) $(CFLAGS) $(HEADERS) -o $@ $< -l$(LIBNAME) -L./lib

clean:
	rm -rf bin/*
	rm -rf lib/*
	rm -rf objects/*

