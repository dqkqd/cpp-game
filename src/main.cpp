#include "SDL_blendmode.h"
#include "SDL_error.h"
#include "SDL_log.h"
#include "SDL_render.h"
#include <SDL3/SDL.h>
#include <SDL3_image/SDL_image.h>
#include <cstddef>
#include <cstdint>
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

const int WALKING_ANIMATION_FRAMES = 4;
SDL_FRect spriteCLips[WALKING_ANIMATION_FRAMES];
LTexture spriteTexture;

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

  if (!spriteTexture.loadFromFile(
          "assets/14_animated_sprites_and_vsync/foo.png")) {
    SDL_Log("%s", SDL_GetError());
    success = false;
  } else {
    spriteCLips[0].x = 0;
    spriteCLips[0].y = 0;
    spriteCLips[0].w = 64;
    spriteCLips[0].h = 205;

    spriteCLips[1].x = 64;
    spriteCLips[1].y = 0;
    spriteCLips[1].w = 64;
    spriteCLips[1].h = 205;

    spriteCLips[2].x = 128;
    spriteCLips[2].y = 0;
    spriteCLips[2].w = 64;
    spriteCLips[2].h = 205;

    spriteCLips[3].x = 192;
    spriteCLips[3].y = 0;
    spriteCLips[3].w = 64;
    spriteCLips[3].h = 205;
  }

  return success;
}

void gameLoop() {

  SDL_Event event;

  bool quit = false;
  int frame = 0;

  while (!quit) {
    while (SDL_PollEvent(&event) != 0) {
      if (event.type == SDL_EVENT_QUIT) {
        quit = true;
        break;
      }
    }

    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    SDL_RenderClear(renderer);

    auto currentClip = &spriteCLips[frame / 4];
    spriteTexture.render((SCREEN_WIDTH - currentClip->w) / 2,
                         (SCREEN_HEIGHT - currentClip->h) / 2, currentClip);
    SDL_RenderPresent(renderer);

    ++frame;
    if (frame / 4 >= WALKING_ANIMATION_FRAMES) {
      frame = 0;
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