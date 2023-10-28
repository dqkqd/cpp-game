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

  void render(int x, int y, SDL_FRect *clip = NULL) {
    SDL_FRect viewport{float(x), float(y), float(width_), float(height_)};
    if (clip != NULL) {
      viewport.w = clip->w;
      viewport.h = clip->h;
    }
    SDL_RenderTexture(renderer, texture_, clip, &viewport);
  }

private:
  SDL_Texture *texture_;
  int width_;
  int height_;
};

SDL_Surface *screenSurface = NULL;
SDL_Surface *currentSurface = NULL;

SDL_FRect spriteClips[4];
LTexture spriteSheetTexture;

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
  SDL_DestroyRenderer(renderer);
  SDL_DestroyWindow(window);
  SDL_Quit();
}

bool loadMedia() {
  bool success = true;

  if (!spriteSheetTexture.loadFromFile(
          "assets/11_clip_rendering_and_sprite_sheets/dots.png")) {
    success = false;
  } else {
    spriteClips[0].x = 0;
    spriteClips[0].y = 0;
    spriteClips[0].w = 100;
    spriteClips[0].h = 100;

    spriteClips[1].x = 100;
    spriteClips[1].y = 0;
    spriteClips[1].w = 100;
    spriteClips[1].h = 100;

    spriteClips[2].x = 0;
    spriteClips[2].y = 100;
    spriteClips[2].w = 100;
    spriteClips[2].h = 100;

    spriteClips[3].x = 100;
    spriteClips[3].y = 100;
    spriteClips[3].w = 100;
    spriteClips[3].h = 100;
  }

  return success;
}

void gameLoop() {

  SDL_Event event;

  bool quit = false;
  while (!quit) {
    while (SDL_PollEvent(&event)) {
      if (event.type == SDL_EVENT_QUIT) {
        quit = true;
        break;
      }

      SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
      SDL_RenderClear(renderer);
      spriteSheetTexture.render(0, 0, &spriteClips[0]);
      spriteSheetTexture.render(SCREEN_WIDTH - spriteClips[1].w, 0,
                                &spriteClips[1]);
      spriteSheetTexture.render(0, SCREEN_HEIGHT - spriteClips[2].h,
                                &spriteClips[2]);
      spriteSheetTexture.render(SCREEN_WIDTH - spriteClips[3].w,
                                SCREEN_HEIGHT - spriteClips[3].h,
                                &spriteClips[3]);
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