CC=clang
CFLAGS= -g -ansi -pedantic -Wall
LD=ld
LDFLAGS= -lpthread
HEADERS=-I./include/
LIBNAME=hny
SOURCES=$(wildcard sources/*.c)
OBJECTS=$(patsubst sources/%.c, objects/%.o, $(SOURCES))
ifeq ($(shell uname), Darwin)
SHAREDFLAG=-dylib -macosx_version_min 10.13
LIB=./lib/lib$(LIBNAME).dylib
else
SHAREDFLAG=-shared
LIB=./lib/lib$(LIBNAME).so
endif
BIN=./bin/$(LIBNAME)

all: $(BIN)

objects/%.o: sources/%.c
	$(CC) $(CFLAGS) $(HEADERS) -c -fPIC -o $@ $<

$(OBJECTS):	$(SOURCES)

$(LIB): $(OBJECTS)
	$(LD) $(SHAREDFLAG) -o $@ $^ $(LDFLAGS)

$(BIN): $(LIB) $(wildcard main.c)
	$(CC) $(CFLAGS) $(HEADERS) -o $@ $^ -l$(LIBNAME) -L./lib

clean:
	rm -rf bin/*
	rm -rf lib/*
	rm -rf objects/*

