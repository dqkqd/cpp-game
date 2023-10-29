#include <SDL3/SDL.h>
#include <SDL3_image/SDL_image.h>
#include <SDL3_ttf/SDL_ttf.h>

#include <cstddef>
#include <cstdint>
#include <string>

#include "SDL_blendmode.h"
#include "SDL_error.h"
#include "SDL_events.h"
#include "SDL_log.h"
#include "SDL_render.h"
#include "SDL_surface.h"

constexpr int SCREEN_WIDTH = 640;
constexpr int SCREEN_HEIGHT = 480;

SDL_Window* window = NULL;
SDL_Renderer* renderer = NULL;
TTF_Font* font = NULL;

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

  bool loadFromRenderedText(std::string textureText, SDL_Color textColor) {
    free();
    auto textSurface =
        TTF_RenderText_Solid(font, textureText.c_str(), textColor);
    if (textSurface == NULL) {
      SDL_Log("%s", TTF_GetError());
    } else {
      texture_ = SDL_CreateTextureFromSurface(renderer, textSurface);
      if (texture_ == NULL) {
        SDL_Log("%s", SDL_GetError());
      } else {
        width_ = textSurface->w;
        height_ = textSurface->h;
      }
      SDL_DestroySurface(textSurface);
    }
    return texture_ != NULL;
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
        }

        if (TTF_Init() < 0) {
          SDL_Log("Couldn't init ttf %s", TTF_GetError());
          success = false;
        }
      }
    }
  }

  return success;
}

void close() {
  currentTexture.free();

  TTF_CloseFont(font);
  SDL_DestroyRenderer(renderer);
  SDL_DestroyWindow(window);

  TTF_Quit();
  IMG_Quit();
  SDL_Quit();
}

bool loadMedia() {
  bool success = true;

  font = TTF_OpenFont("assets/16_true_type_fonts/lazy.ttf", 28);
  if (font == NULL) {
    SDL_Log("%s", TTF_GetError());
    success = false;
  } else {
    SDL_Color textColor{0, 0, 0};
    if (!currentTexture.loadFromRenderedText(
            "The quick brown fox jumps over the lazy dog", textColor)) {
      success = false;
    }
  }

  return success;
}

void gameLoop() {
  SDL_Event event;

  bool quit = false;

  while (!quit) {
    while (SDL_PollEvent(&event) != 0) {
      if (event.type == SDL_EVENT_QUIT) {
        quit = true;
        break;
      }
    }

    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    SDL_RenderClear(renderer);

    currentTexture.render((SCREEN_WIDTH - currentTexture.getWidth()) / 2,
                          (SCREEN_HEIGHT - currentTexture.getHeight()) / 2);

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