#include "SDL_error.h"
#include "SDL_log.h"
#include "SDL_oldnames.h"
#include "SDL_pixels.h"
#include "SDL_rect.h"
#include "SDL_render.h"
#include "SDL_stdinc.h"
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

class LTexture {
public:
  LTexture() {}
  ~LTexture() { free(); }

  void free() {
    if (texture_ != NULL) {
      SDL_DestroyTexture(texture_);
      texture_ = NULL;
      width_ = 0;
      height_ = 0;
    }
  }

  bool loadFromFile(std::string path) {
    free();
    auto surface = IMG_Load(path.c_str());
    if (surface == NULL) {
      SDL_Log("%s", IMG_GetError());
      return false;
    }
    SDL_SetSurfaceColorKey(surface, SDL_TRUE,
                           SDL_MapRGB(surface->format, 0, 255, 255));
    texture_ = SDL_CreateTextureFromSurface(renderer, surface);
    if (texture_ == NULL) {
      SDL_Log("%s", IMG_GetError());
      return false;
    }
    width_ = surface->w;
    height_ = surface->h;

    SDL_DestroySurface(surface);

    return true;
  }

  void render(int x, int y) {
    SDL_FRect viewport{float(x), float(y), float(width_), float(height_)};
    SDL_RenderTexture(renderer, texture_, NULL, &viewport);
  }

private:
  SDL_Texture *texture_;
  int width_;
  int height_;
};

SDL_Surface *screenSurface = NULL;
SDL_Surface *keyPressSurfaces[static_cast<size_t>(
    KeyPressSurfaces::KEY_PRESS_SURFACE_TOTAL)];
SDL_Surface *currentSurface = NULL;

SDL_Texture *currentTexture = NULL;

LTexture fooTexture;
LTexture backgroundTexture;

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

  fooTexture.loadFromFile("assets/foo.png");
  backgroundTexture.loadFromFile("assets/background.png");
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

      SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
      SDL_RenderClear(renderer);

      backgroundTexture.render(0, 0);
      fooTexture.render(240, 190);

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