CFLAGS=-Wall -Wextra -std=c17

SDL2=`sdl2-config --cflags --libs`

SDL2_PATH=-I/opt/homebrew/include/SDL2

# Funny & leanring moment: named external directory as 'external ' with space and was so frustrated as 
# the damn clang compiler was not detecting the file
INCLUDES=-I. -Iexternal -Isrc

TARGET=grive

SRC:=$(wildcard src/*.c) 

# filter out line.c for a while 
# SRC:=$(filter-out src/line.c, $(SRC))

# OBJ=$(SRC:.c=.o)
OBJ=$(patsubst src/%.c, build/%.o, $(SRC))

build/%.o: src/%.c
	$(CC) $(CFLAGS) $(INCLUDES) $(SDL2_PATH) -c -o $@ $<

grive: $(OBJ)
	$(CC) $(CFLAGS) $(INCLUDES) $(SDL2) -o $(TARGET) $^

clean:
	rm -f $(TARGET) *.o

.PHONY: 
	clean 