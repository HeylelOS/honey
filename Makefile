CC=clang
CFLAGS= -g -ansi -pedantic -Wall
LD=ld
LDFLAGS= -lpthread
HEADERS=-I./include/
LIBNAME=hny
OBJECT=./objects/$(LIBNAME).o
ifeq ($(shell uname), Darwin)
SHAREDFLAG=-dylib -macosx_version_min 10.13
LIB=./lib/lib$(LIBNAME).dylib
else
SHAREDFLAG=-shared
LIB=./lib/lib$(LIBNAME).so
endif
BIN=./bin/$(LIBNAME)

all: $(BIN)

$(OBJECT): $(wildcard sources/*.c)
	$(CC) $(CFLAGS) $(HEADERS) -c -fPIC -o $@ $^

$(LIB): $(OBJECT)
	$(LD) $(SHAREDFLAG) -o $@ $^ $(LDFLAGS)

$(BIN): $(LIB) $(wildcard main.c)
	$(CC) $(CFLAGS) $(HEADERS) -o $@ $^ -l$(LIBNAME) -L./lib

clean:
	rm -rf bin/*
	rm -rf lib/*
	rm -rf objects/*

