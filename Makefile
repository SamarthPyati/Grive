CFLAGS=-Wall -Wextra -std=c17

SDL2=`sdl2-config --cflags --libs`
GLEW=`pkg-config --libs --cflags glew`
GLFW=`pkg-config --libs --cflags glfw3`

SDL2_PATH=/opt/homebrew/include/SDL2
GLEW_PATH=/opt/homebrew/Cellar/glew/2.2.0_1/include
GLFW_PATH=/opt/homebrew/Cellar/glfw/3.4/include

# Funny & leanring moment: named external directory as 'external ' with space and was so frustrated as 
# the damn clang compiler was not detecting the file
INCLUDES=-I. -Iexternal -Isrc -I$(SDL2_PATH) -I$(GLEW_PATH) -I$(GLFW_PATH)

# Requires on mac 
FRAMEWORK=-framework OpenGL

TARGET=grive

SRC:=$(wildcard src/*.c) 

# filter out line.c for a while 
# SRC:=$(filter-out src/line.c, $(SRC))

# OBJ=$(SRC:.c=.o)
OBJ=$(patsubst src/%.c, build/%.o, $(SRC))

build/%.o: src/%.c
	$(CC) $(CFLAGS) $(INCLUDES) -c -o $@ $<

grive: $(OBJ)
	$(CC) $(CFLAGS) $(INCLUDES) $(SDL2) $(GLEW) $(GLFW) $(FRAMEWORK) -o $(TARGET) $^

clean:
	rm -f $(TARGET) *.o

.PHONY: 
	clean 