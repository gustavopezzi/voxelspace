#include <stddef.h>
#include <stdlib.h>
#include <stdint.h>
#include <math.h>
#include "dos/dos.h"

#define SCREEN_WIDTH 320
#define SCREEN_HEIGHT 200
#define MAP_N 1024
#define SCALE_FACTOR 70.0

///////////////////////////////////////////////////////////////////////////////
// Buffers for Heightmap and Colormap
///////////////////////////////////////////////////////////////////////////////
uint8_t* heightmap = NULL;   // Buffer/array to hold height values (1024*1024)
uint8_t* colormap  = NULL;   // Buffer/array to hold color values  (1024*1024)

///////////////////////////////////////////////////////////////////////////////
// Camera struct type declaration
///////////////////////////////////////////////////////////////////////////////
typedef struct {
  float x;         // x position on the map
  float y;         // y position on the map
  float height;    // height of the camera
  float horizon;   // offset of the horizon position (looking up-down)
  float zfar;      // distance of the camera looking forward
  float angle;     // camera angle (radians, clockwise)
} camera_t;

///////////////////////////////////////////////////////////////////////////////
// Camera initialization
///////////////////////////////////////////////////////////////////////////////
camera_t camera = {
  .x       = 512.0,
  .y       = 512.0,
  .height  = 70.0,
  .horizon = 60.0,
  .zfar    = 600.0,
  .angle   = 1.5 * 3.141592 // (= 270 deg)
};

///////////////////////////////////////////////////////////////////////////////
// Handle keyboard input
///////////////////////////////////////////////////////////////////////////////
void processinput() {
  if (keystate(KEY_UP)) {
    camera.x += cos(camera.angle);
    camera.y += sin(camera.angle);
  }
  if (keystate(KEY_DOWN)) {
    camera.x -= cos(camera.angle);
    camera.y -= sin(camera.angle);
  }
  if (keystate(KEY_LEFT)) {
    camera.angle -= 0.01;
  }
  if (keystate(KEY_RIGHT)) {
    camera.angle += 0.01;
  }
  if (keystate(KEY_E)) {
    camera.height++;
  }
  if (keystate(KEY_D)) {
    camera.height--;
  }
  if (keystate(KEY_S)) {
    camera.horizon += 1.5;
  }
  if (keystate(KEY_W)) {
    camera.horizon -= 1.5;
  }
}

///////////////////////////////////////////////////////////////////////////////
// Main function
///////////////////////////////////////////////////////////////////////////////
int main(int argc, char* args[]) {
  setvideomode(videomode_320x200);

  // Declare an array to hold the max. number of possible colors (*3 for RGB)
  uint8_t palette[256 * 3];
  int palsize;

  // Load the colormap, heightmap, and palette from the external GIF files
  colormap = loadgif("maps/gif/map0.color.gif", NULL, NULL, &palsize, palette);
  heightmap = loadgif("maps/gif/map0.height.gif", NULL, NULL, NULL, NULL);

  // Load the system color palette based on the colors found in the GIF file
  for (int i = 0; i < palsize; i++) {
    setpal(i, palette[3 * i + 0], palette[3 * i + 1], palette[3 * i + 2]);
  }

  // Set the default background color of the system (palette index 0)
  // setpal(0, 36, 36, 56);

  setdoublebuffer(1);

  // Allocate a new framebuffer for the current screen (320*200 bytes)
  uint8_t* framebuffer = screenbuffer();

  // Game loop
  while (!shuttingdown()) {
    waitvbl();
    clearscreen();

    processinput();
  
    float sinangle = sin(camera.angle);
    float cosangle = cos(camera.angle);

    // Left-most point of the FOV
    float plx = cosangle * camera.zfar + sinangle * camera.zfar;
    float ply = sinangle * camera.zfar - cosangle * camera.zfar;

    // Right-most point of the FOV
    float prx = cosangle * camera.zfar - sinangle * camera.zfar;
    float pry = sinangle * camera.zfar + cosangle * camera.zfar;

    // Loop 320 rays from left to right
    for (int i = 0; i < SCREEN_WIDTH; i++) {
      float deltax = (plx + (prx - plx) / SCREEN_WIDTH * i) / camera.zfar;
      float deltay = (ply + (pry - ply) / SCREEN_WIDTH * i) / camera.zfar;

      // Ray (x,y) coords
      float rx = camera.x;
      float ry = camera.y;

      // Store the tallest projected height per-ray
      float tallestheight = SCREEN_HEIGHT;

      // Loop all depth units until the zfar distance limit
      for (int z = 1; z < camera.zfar; z++) {
        rx += deltax;
        ry += deltay;

        // Find the offset that we have to go and fetch values from the heightmap
        int mapoffset = ((MAP_N * ((int)(ry) & (MAP_N - 1))) + ((int)(rx) & (MAP_N - 1)));

        // Project height values and find the height on-screen
        int projheight = (int)((camera.height - heightmap[mapoffset]) / z * SCALE_FACTOR + camera.horizon);

        // Only draw pixels if the new projected height is taller than the previous tallest height
        if (projheight < tallestheight) {
          // Draw pixels from previous max-height until the new projected height
          for (int y = projheight; y < tallestheight; y++) {
            if (y >= 0) {
              framebuffer[(SCREEN_WIDTH * y) + i] = colormap[mapoffset];
            }
          }
          tallestheight = projheight;
        }
      }
    }

    framebuffer = swapbuffers();

    if (keystate(KEY_ESCAPE))
      break;
  }

  return EXIT_SUCCESS;
}
