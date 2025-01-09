EXECUTABLE = 412alloc
TARNAME = csn2.tar
CC = gcc
CFLAGS = -Wall -g

# Source files
SOURCES = realloc.c scanner.c
OBJECTS = $(SOURCES:.c=.o)

# Header files
HEADERS = ir.h

# Files to include in tar
FILES = $(SOURCES) $(HEADERS) makefile README.md

all: build

build: $(EXECUTABLE)

# Link the final executable
$(EXECUTABLE): $(OBJECTS)
	$(CC) $(CFLAGS) -o $(EXECUTABLE) $(OBJECTS)

# Compile source files to object files
%.o: %.c $(HEADERS)
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(EXECUTABLE) $(OBJECTS) *~ core core.* vgcore.* $(TARNAME)

tar: $(FILES)
	tar -cvf $(TARNAME) $(FILES)

.PHONY: all build clean tar