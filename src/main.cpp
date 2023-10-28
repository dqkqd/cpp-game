#include "SDL_error.h"
#include "SDL_log.h"
#include "SDL_oldnames.h"
#include "SDL_rect.h"
#include "SDL_render.h"
#include "SDL_surface.h"
#include <SDL3/SDL.h>
#include <SDL3_image/SDL_image.h>
#include <cstddef>
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
SDL_Renderer *renderer = NULL;

SDL_Surface *screenSurface = NULL;
SDL_Surface *keyPressSurfaces[static_cast<size_t>(
    KeyPressSurfaces::KEY_PRESS_SURFACE_TOTAL)];
SDL_Surface *currentSurface = NULL;

SDL_Texture *currentTexture = NULL;

constexpr int SCREEN_WIDTH = 640;
constexpr int SCREEN_HEIGHT = 480;

bool init() {

  bool success = true;

  if (SDL_Init(SDL_INIT_VIDEO) < 0) {
    SDL_Log("SDL_Init failed (%s)", SDL_GetError());
    success = false;
  } else {

    window = SDL_CreateWindow("Hello SDL", SCREEN_WIDTH, SCREEN_HEIGHT,
                              SDL_WINDOW_OPENGL);
    if (!window) {
      SDL_Log("%s", SDL_GetError());
      success = false;
    } else {

      renderer = SDL_CreateRenderer(window, NULL, SDL_RENDERER_ACCELERATED);
      if (!renderer) {
        SDL_Log("%s", SDL_GetError());
        success = false;
      } else {
        int imgFlags = IMG_INIT_PNG;
        if (!(IMG_Init(imgFlags) & imgFlags)) {
          SDL_Log("%s ", IMG_GetError());
          success = false;
        } else {
          screenSurface = SDL_GetWindowSurface(window);
        }
      }
    }
  }

  return success;
}

void close() {
  SDL_DestroySurface(screenSurface);
  SDL_DestroyTexture(currentTexture);
  SDL_DestroyRenderer(renderer);
  SDL_DestroyWindow(window);
  SDL_Quit();
}

SDL_Surface *loadSurface(std::string path) {
  auto loadedSurface = IMG_Load(path.c_str());
  if (!loadedSurface) {
    SDL_Log("%s %s", path.c_str(), IMG_GetError());
    return nullptr;
  }

  auto optimizedSurface =
      SDL_ConvertSurface(loadedSurface, loadedSurface->format);
  SDL_DestroySurface(loadedSurface);

  return optimizedSurface;
}

SDL_Texture *loadTexture(std::string path) {
  auto texture = IMG_LoadTexture(renderer, path.c_str());
  if (!texture) {
    SDL_Log("%s", IMG_GetError());
  }
  return texture;
}

bool loadMedia() {
  bool success = true;

  currentTexture = loadTexture("assets/texture.png");
  if (!currentTexture) {
    success = false;
  }

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

      SDL_RenderClear(renderer);

      SDL_Rect topLeftViewport{0, 0, SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2};
      SDL_SetRenderViewport(renderer, &topLeftViewport);
      SDL_RenderTexture(renderer, currentTexture, NULL, NULL);

      SDL_Rect topRightViewport{SCREEN_WIDTH / 2, 0, SCREEN_WIDTH / 2,
                                SCREEN_HEIGHT / 2};
      SDL_SetRenderViewport(renderer, &topRightViewport);
      SDL_RenderTexture(renderer, currentTexture, NULL, NULL);

      SDL_Rect bottomViewport{0, SCREEN_HEIGHT / 2, SCREEN_WIDTH,
                              SCREEN_HEIGHT / 2};
      SDL_SetRenderViewport(renderer, &bottomViewport);
      SDL_RenderTexture(renderer, currentTexture, NULL, NULL);

      SDL_RenderPresent(renderer);
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