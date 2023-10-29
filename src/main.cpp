#include <SDL3/SDL.h>
#include <SDL3_image/SDL_image.h>

#include <cstddef>
#include <cstdint>
#include <string>

#include "SDL_blendmode.h"
#include "SDL_error.h"
#include "SDL_events.h"
#include "SDL_log.h"
#include "SDL_render.h"

enum class KeyPressSurfaces {
  KEY_PRESS_SURFACE_DEFAULT,
  KEY_PRESS_SURFACE_UP,
  KEY_PRESS_SURFACE_DOWN,
  KEY_PRESS_SURFACE_LEFT,
  KEY_PRESS_SURFACE_RIGHT,
  KEY_PRESS_SURFACE_TOTAL
};

SDL_Window* window = NULL;
SDL_Renderer* renderer = NULL;

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

  void setColor(Uint8 r, Uint8 g, Uint8 b) {
    if (SDL_SetTextureColorMod(texture_, r, g, b) < 0) {
      SDL_Log("%s", SDL_GetError());
    }
  }

  void setBlendMode(SDL_BlendMode blending) {
    SDL_SetTextureBlendMode(texture_, blending);
  }

  void setAlpha(uint8_t alpha) { SDL_SetTextureAlphaMod(texture_, alpha); }

  bool loadFromFile(std::string path) {
    free();
    texture_ = IMG_LoadTexture(renderer, path.c_str());
    if (texture_ == NULL) {
      SDL_Log("%s", IMG_GetError());
    } else if (SDL_QueryTexture(texture_, NULL, NULL, &width_, &height_) < 0) {
      SDL_Log("%s", SDL_GetError());
    }
    return true;
  }

  int getWidth() { return width_; }
  int getHeight() { return height_; }

  void render(int x, int y, SDL_FRect* clip = NULL, double angle = 0.0,
              SDL_FPoint* center = NULL,
              SDL_RendererFlip flip = SDL_FLIP_NONE) {
    SDL_FRect viewport{float(x), float(y), float(width_), float(height_)};
    if (clip != NULL) {
      viewport.w = clip->w;
      viewport.h = clip->h;
    }
    SDL_RenderTextureRotated(renderer, texture_, clip, &viewport, angle, center,
                             flip);
  }

 private:
  SDL_Texture* texture_;
  int width_;
  int height_;
};

SDL_Surface* screenSurface = NULL;

constexpr int SCREEN_WIDTH = 640;
constexpr int SCREEN_HEIGHT = 480;

LTexture currentTexture;

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
      renderer = SDL_CreateRenderer(
          window, NULL, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
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

  if (!currentTexture.loadFromFile(
          "assets/15_rotation_and_flipping/arrow.png")) {
    SDL_Log("%s", SDL_GetError());
    success = false;
  }

  return success;
}

void gameLoop() {
  SDL_Event event;

  bool quit = false;
  double degrees = 0;
  SDL_RendererFlip flipType = SDL_FLIP_NONE;

  while (!quit) {
    while (SDL_PollEvent(&event) != 0) {
      if (event.type == SDL_EVENT_QUIT) {
        quit = true;
        break;
      } else if (event.type == SDL_EVENT_KEY_DOWN) {
        switch (event.key.keysym.sym) {
          case SDLK_a:
            degrees -= 60;
            break;
          case SDLK_d:
            degrees += 60;
            break;
          case SDLK_q:
            flipType = SDL_FLIP_HORIZONTAL;
            break;
          case SDLK_w:
            flipType = SDL_FLIP_NONE;
            break;
          case SDLK_e:
            flipType = SDL_FLIP_VERTICAL;
            break;
        }
      }
    }

    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    SDL_RenderClear(renderer);

    currentTexture.render((SCREEN_WIDTH - currentTexture.getWidth()) / 2,
                          (SCREEN_HEIGHT - currentTexture.getHeight()) / 2,
                          NULL, degrees, NULL, flipType);
    SDL_RenderPresent(renderer);

    if (quit) {
      break;
    }
  }
}

int main(int argc, char* argv[]) {
  init();
  if (loadMedia()) {
    gameLoop();
  }
  close();
  return 0;
}