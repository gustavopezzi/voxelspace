OS := $(shell uname -s)

ifeq ($(OS),Darwin)
	CFLAGS := `sdl2-config --libs --cflags` -lGLEW -framework OpenGL -lpthread
else
	CFLAGS := `sdl2-config --libs --cflags` -lGLEW -lGL -lm -lpthread
endif

build:
	gcc voxel.c dos/*.c $(CFLAGS) -o voxel

run:
	./voxel

clean:
	rm voxel