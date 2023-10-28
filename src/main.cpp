#include "SDL_error.h"
#include "SDL_events.h"
#include "SDL_init.h"
#include "SDL_keycode.h"
#include "SDL_log.h"
#include "SDL_oldnames.h"
#include "SDL_pixels.h"
#include "SDL_surface.h"
#include "SDL_video.h"
#include <SDL3/SDL.h>
#include <string>

enum class KeyPressSurfaces {
  KEY_PRESS_SURFACE_DEFAULT,
  KEY_PRESS_SURFACE_UP,
  KEY_PRESS_SURFACE_DOWN,
  KEY_PRESS_SURFACE_LEFT,
  KEY_PRESS_SURFACE_RIGHT,
  KEY_PRESS_SURFACE_TOTAL
};

SDL_Window *window = NULL;
SDL_Surface *screenSurface = NULL;
SDL_Surface *image = NULL;
SDL_Surface *keyPressSurfaces[static_cast<size_t>(
    KeyPressSurfaces::KEY_PRESS_SURFACE_TOTAL)];
SDL_Surface *currentSurface = NULL;

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

SDL_Surface *loadSurface(std::string path) {
  auto loadedSurface = SDL_LoadBMP(path.c_str());
  if (!loadedSurface) {
    SDL_Log("%s %s", path.c_str(), SDL_GetError());
    return nullptr;
  }

  auto optimizedSurface =
      SDL_ConvertSurface(loadedSurface, loadedSurface->format);
  SDL_DestroySurface(loadedSurface);

  return optimizedSurface;
}

bool loadMedia() {
  bool success = true;

  // default
  keyPressSurfaces[int(KeyPressSurfaces::KEY_PRESS_SURFACE_DEFAULT)] =
      loadSurface("assets/press.bmp");
  if (!keyPressSurfaces[int(KeyPressSurfaces::KEY_PRESS_SURFACE_DEFAULT)]) {
    success = false;
  }

  // up
  keyPressSurfaces[int(KeyPressSurfaces::KEY_PRESS_SURFACE_UP)] =
      loadSurface("assets/up.bmp");
  if (!keyPressSurfaces[int(KeyPressSurfaces::KEY_PRESS_SURFACE_UP)]) {
    success = false;
  }

  // down
  keyPressSurfaces[int(KeyPressSurfaces::KEY_PRESS_SURFACE_DOWN)] =
      loadSurface("assets/down.bmp");
  if (!keyPressSurfaces[int(KeyPressSurfaces::KEY_PRESS_SURFACE_DOWN)]) {
    success = false;
  }

  // left
  keyPressSurfaces[int(KeyPressSurfaces::KEY_PRESS_SURFACE_LEFT)] =
      loadSurface("assets/left.bmp");
  if (!keyPressSurfaces[int(KeyPressSurfaces::KEY_PRESS_SURFACE_LEFT)]) {
    success = false;
  }

  // right
  keyPressSurfaces[int(KeyPressSurfaces::KEY_PRESS_SURFACE_RIGHT)] =
      loadSurface("assets/right.bmp");
  if (!keyPressSurfaces[int(KeyPressSurfaces::KEY_PRESS_SURFACE_RIGHT)]) {
    success = false;
  }

  return success;
}

void gameLoop() {

  SDL_Event event;
  currentSurface =
      keyPressSurfaces[int(KeyPressSurfaces::KEY_PRESS_SURFACE_DEFAULT)];

  bool quit = false;
  while (!quit) {
    while (SDL_PollEvent(&event)) {
      if (event.type == SDL_EVENT_QUIT) {
        quit = true;
        break;
      }

      // user presses a key
      else if (event.type == SDL_EVENT_KEY_DOWN) {
        switch (event.key.keysym.sym) {
        case SDLK_UP:
          currentSurface =
              keyPressSurfaces[int(KeyPressSurfaces::KEY_PRESS_SURFACE_UP)];
          break;
        case SDLK_DOWN:
          currentSurface =
              keyPressSurfaces[int(KeyPressSurfaces::KEY_PRESS_SURFACE_DOWN)];
          break;
        case SDLK_LEFT:
          currentSurface =
              keyPressSurfaces[int(KeyPressSurfaces::KEY_PRESS_SURFACE_LEFT)];
          break;
        case SDLK_RIGHT:
          currentSurface =
              keyPressSurfaces[int(KeyPressSurfaces::KEY_PRESS_SURFACE_RIGHT)];
          break;
        default:
          currentSurface = keyPressSurfaces[int(
              KeyPressSurfaces::KEY_PRESS_SURFACE_DEFAULT)];
          break;
        }
      }

      SDL_BlitSurface(currentSurface, nullptr, screenSurface, nullptr);
      SDL_UpdateWindowSurface(window);
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