CC=clang
CFLAGS= -g -ansi -pedantic -Wall
LD=ld
LDFLAGS= -lcurl -ljson-c -lpthread
HEADERS=-I./headers/
LIBNAME=hny
OBJECTS=./objects/$(LIBNAME).o
LIB=./lib/lib$(LIBNAME).so
BIN=./bin/$(LIBNAME)

all: $(BIN)

$(OBJECTS): $(wildcard sources/*.c)
	$(CC) $(CFLAGS) $(HEADERS) -c -fPIC -o $@ $^

$(LIB): $(OBJECTS)
	$(LD) -shared -o $@ $^ $(LDFLAGS)

$(BIN): $(LIB) $(wildcard main.c)
	$(CC) $(CFLAGS) $(HEADERS) -o $@ $^ -l$(LIBNAME) -L./lib
