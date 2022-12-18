build:
	gcc voxel.c dos/*.c `sdl2-config --libs --cflags` -lGLEW -framework OpenGL -lpthread -o voxel

run:
	./voxel

clean:
	rm voxel