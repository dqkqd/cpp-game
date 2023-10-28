#include "SDL_error.h"
#include "SDL_init.h"
#include "SDL_log.h"
#include "SDL_oldnames.h"
#include "SDL_pixels.h"
#include "SDL_surface.h"
#include "SDL_video.h"
#include <SDL3/SDL.h>

SDL_Window *window = NULL;
SDL_Surface *screenSurface = NULL;
SDL_Surface *image = NULL;

bool init() {

  bool success = true;

  if (SDL_Init(SDL_INIT_VIDEO) < 0) {
    SDL_Log("SDL_Init failed (%s)", SDL_GetError());
    success = false;
  } else {

    window = SDL_CreateWindow("Hello SDL", 640, 480, SDL_WINDOW_OPENGL);
    if (!window) {
      SDL_Log("%s", SDL_GetError());
      success = false;
    } else {
      screenSurface = SDL_GetWindowSurface(window);
      // SDL_FillSurfaceRect(screenSurface, nullptr,
      //                     SDL_MapRGB(screenSurface->format, 0xFF, 0xFF,
      //                     0xFF));
      // SDL_UpdateWindowSurface(window);
    }
  }

  return success;
}

void close() {
  SDL_DestroySurface(screenSurface);
  SDL_DestroySurface(image);
  SDL_DestroyWindow(window);
  SDL_Quit();
}

bool loadMedia() {
  bool success = true;
  image = SDL_LoadBMP("assets/hello_world.bmp");
  if (!image) {
    SDL_Log("%s", SDL_GetError());
    success = false;
  }
  return success;
}

void gameLoop() {
  SDL_BlitSurface(image, nullptr, screenSurface, nullptr);
  SDL_UpdateWindowSurface(window);

  bool quit = false;
  while (!quit) {
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
      if (event.type == SDL_EVENT_QUIT) {
        quit = true;
        break;
      }
    }
    if (quit) {
      break;
    }
  }
}

int main(int argc, char *argv[]) {
  init();
  if (loadMedia()) {
    gameLoop();
  }
  close();
  return 0;
}