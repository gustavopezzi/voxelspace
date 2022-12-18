#include <stddef.h>
#include <stdlib.h>
#include <stdint.h>
#include <math.h>
#include "dos/dos.h"

#define SCREEN_WIDTH 320
#define SCREEN_HEIGHT 200
#define SCALE_FACTOR 100.0

/*****************************************************************************/
/* BUFFERS FOR HEIGHTMAP & COLORMAP                                          */
/*****************************************************************************/
uint8_t* heightmap = NULL; // Buffer/array to hold height values (1024*1024)
uint8_t* colormap  = NULL; // Buffer/array to hold color values  (1024*1024)

/*****************************************************************************/
/* CAMERA DECLARATION                                                        */
/*****************************************************************************/
typedef struct {
  float x;       // x position on the map
  float y;       // y position on the map
  float height;  // height of the camera
  float horizon; // offset of the horizon position (looking up-down)
  float zfar;    // distance of the camera looking forward
  float angle;   // camera angle (radians, clockwise)
} camera_t;

camera_t camera = {
  .x       = 512.0,
  .y       = 512.0,
  .height  = 80.0,
  .horizon = 50.0,
  .zfar    = 600.0,
  .angle   = 1.5 * 3.141592 
};

/*****************************************************************************/
/* PROCESS KEYBOARD INPUT                                                    */
/*****************************************************************************/
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

/*****************************************************************************/
/* MAIN                                                                      */
/*****************************************************************************/
int main(int argc, char* args[]) {
  setvideomode(videomode_320x200);

  uint8_t palette[256 * 3];
  int map_width, map_height, pal_count;

  colormap = loadgif("maps/map0.color.gif", NULL, NULL, &pal_count, palette);
  heightmap = loadgif("maps/map0.height.gif", NULL, NULL, NULL, NULL);

  for (int i = 0; i < pal_count; i++) {
    setpal(i, palette[3 * i + 0], palette[3 * i + 1], palette[3 * i + 2]);
  }
  setpal(0, 36, 36, 56);
  
  setdoublebuffer(1);
  uint8_t* framebuffer = screenbuffer();

  while (!shuttingdown()) {
    waitvbl();
    clearscreen();

    processinput();
  
    float sinangle = sin(camera.angle);
    float cosangle = cos(camera.angle);

    float plx = cosangle * camera.zfar + sinangle * camera.zfar;
    float ply = sinangle * camera.zfar - cosangle * camera.zfar;

    float prx = cosangle * camera.zfar - sinangle * camera.zfar;
    float pry = sinangle * camera.zfar + cosangle * camera.zfar;

    // Loop 320 rays from left to right
    for (int i = 0; i < SCREEN_WIDTH; i++) {
      float delta_x = (plx + (prx - plx) / SCREEN_WIDTH * i) / camera.zfar;
      float delta_y = (ply + (pry - ply) / SCREEN_WIDTH * i) / camera.zfar;

      float rx = camera.x;
      float ry = camera.y;

      float max_height = SCREEN_HEIGHT;

      for (int z = 1; z < camera.zfar; z++) {
        rx += delta_x;
        ry += delta_y;

        // Find the offset that we have to go and fetch values from the heightmap
        int mapoffset = ((1024 * ((int)(ry) & 1023)) + ((int)(rx) & 1023));

        // Project heights and find the height on-screen
        int proj_height = (int)((camera.height - heightmap[mapoffset]) / z * SCALE_FACTOR + camera.horizon);

        // Prevent projected height value to always be between 0 and screen height
        proj_height = (proj_height < 0) ? 0 : proj_height;
        proj_height = (proj_height >= SCREEN_HEIGHT) ? SCREEN_HEIGHT - 1 : proj_height;

        // Only render terrain pixels if the new projected height is taller than the previous max-height
        if (proj_height < max_height) {
          // Draw pixels from previous max-height until the new projected height
          for (int y = proj_height; y < max_height; y++) {
            framebuffer[(SCREEN_WIDTH * y) + i] = (uint8_t)colormap[mapoffset];
          }
          max_height = proj_height;
        }
      }
    }

    framebuffer = swapbuffers();

    if (keystate(KEY_ESCAPE))
      break;
  }

  free(colormap);
  free(heightmap);

  return EXIT_SUCCESS;
}
