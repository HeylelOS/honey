CC=clang
CFLAGS= -g -ansi -pedantic -Wall
LD=ld
ifeq ($(shell uname), Darwin)
SHAREDFLAG=-dylib -macosx_version_min 10.13
else
SHAREDFLAG=-shared
endif
LDFLAGS= -lpthread
HEADERS=-I./include/
LIBNAME=hny
OBJECTS=./objects/$(LIBNAME).o
LIB=./lib/lib$(LIBNAME).so
BIN=./bin/$(LIBNAME)

all: $(BIN)

$(OBJECTS): $(wildcard sources/*.c)
	$(CC) $(CFLAGS) $(HEADERS) -c -fPIC -o $@ $^

$(LIB): $(OBJECTS)
	$(LD) $(SHAREDFLAG) -o $@ $^ $(LDFLAGS)

$(BIN): $(LIB) $(wildcard main.c)
	$(CC) $(CFLAGS) $(HEADERS) -o $@ $^ -l$(LIBNAME) -L./lib
