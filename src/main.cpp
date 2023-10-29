#include <SDL3/SDL.h>
#include <SDL3_image/SDL_image.h>
#include <SDL3_mixer/SDL_mixer.h>
#include <SDL3_ttf/SDL_ttf.h>

#include <cstddef>
#include <cstdint>
#include <sstream>
#include <string>

#include "SDL_error.h"
#include "SDL_events.h"
#include "SDL_init.h"
#include "SDL_keyboard.h"
#include "SDL_keycode.h"
#include "SDL_log.h"
#include "SDL_pixels.h"
#include "SDL_render.h"
#include "SDL_scancode.h"
#include "SDL_timer.h"

constexpr int SCREEN_WIDTH = 640;
constexpr int SCREEN_HEIGHT = 480;
constexpr int SCREEN_FPS = 60;
constexpr int SCREEN_TICKS_PER_FRAME = 1000 / SCREEN_FPS;

SDL_Window* window = NULL;
SDL_Renderer* renderer = NULL;
TTF_Font* font = NULL;

SDL_AudioSpec spec;

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

LTexture dotTexture;

class LTimer {
 public:
  LTimer() : started_(false), paused_(false), startTicks_(0), pausedTicks_(0) {}

  void start() {
    started_ = true;
    paused_ = false;

    startTicks_ = SDL_GetTicks();
    pausedTicks_ = 0;
  }

  void stop() {
    started_ = false;
    paused_ = false;

    startTicks_ = 0;
    pausedTicks_ = 0;
  }

  void pause() {
    if (started_ && !paused_) {
      paused_ = true;
      pausedTicks_ = SDL_GetTicks() - startTicks_;
      startTicks_ = 0;
    }
  }

  void unpause() {
    if (started_ && paused_) {
      paused_ = false;
      startTicks_ = SDL_GetTicks() - pausedTicks_;
      pausedTicks_ = 0;
    }
  }

  uint32_t getTicks() {
    uint32_t time = 0;
    if (started_) {
      if (paused_) {
        time = pausedTicks_;
      } else {
        time = SDL_GetTicks() - startTicks_;
      }
    }
    return time;
  }

  bool isStarted() { return started_; }
  bool isPaused() { return paused_; }

 private:
  uint32_t startTicks_;
  uint32_t pausedTicks_;
  bool paused_;
  bool started_;
};

class Dot {
 public:
  static constexpr int DOT_WIDTH = 20;
  static constexpr int DOT_HEIGHT = 20;
  static constexpr int DOT_VEL = 10;

  Dot() : posX_(0), posY_(0), velX_(0), velY_(0) {}

  void handleEvent(SDL_Event& e) {
    if (e.type == SDL_EVENT_KEY_DOWN && e.key.repeat == 0) {
      switch (e.key.keysym.sym) {
        case SDLK_UP:
          velY_ -= DOT_VEL;
          break;
        case SDLK_DOWN:
          velY_ += DOT_VEL;
          break;
        case SDLK_LEFT:
          velX_ -= DOT_VEL;
          break;
        case SDLK_RIGHT:
          velX_ += DOT_VEL;
          break;
      }
    } else if (e.type == SDL_EVENT_KEY_UP && e.key.repeat == 0) {
      switch (e.key.keysym.sym) {
        case SDLK_UP:
          velY_ += DOT_VEL;
          break;
        case SDLK_DOWN:
          velY_ -= DOT_VEL;
          break;
        case SDLK_LEFT:
          velX_ += DOT_VEL;
          break;
        case SDLK_RIGHT:
          velX_ -= DOT_VEL;
          break;
      }
    }
  }

  void move() {
    posX_ += velX_;
    if (posX_ < 0 || posX_ + DOT_WIDTH > SCREEN_WIDTH) {
      posX_ -= velX_;
    }

    posY_ += velY_;
    if (posY_ < 0 || posY_ + DOT_HEIGHT > SCREEN_HEIGHT) {
      posY_ -= velY_;
    }
  }

  void render() { dotTexture.render(posX_, posY_); }

 private:
  int posX_;
  int posY_;
  int velX_;
  int velY_;
};

bool init() {
  bool success = true;

  if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) < 0) {
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

        spec.freq = MIX_DEFAULT_FREQUENCY;
        spec.format = MIX_DEFAULT_FORMAT;
        spec.channels = MIX_DEFAULT_CHANNELS;

        if (Mix_OpenAudio(0, &spec) < 0) {
          SDL_Log("Couldn't open audio %s", Mix_GetError());
          success = false;
        }
      }
    }
  }

  return success;
}

void close() {
  dotTexture.free();

  Mix_CloseAudio();

  TTF_CloseFont(font);
  SDL_DestroyRenderer(renderer);
  SDL_DestroyWindow(window);

  Mix_Quit();
  TTF_Quit();
  IMG_Quit();
  SDL_Quit();
}

bool loadMedia() {
  bool success = true;
  if (!dotTexture.loadFromFile("assets/26_motion/dot.bmp")) {
    success = false;
  }
  return success;
}

void gameLoop() {
  SDL_Event event;

  bool quit = false;
  Dot dot;

  while (!quit) {
    while (SDL_PollEvent(&event) != 0) {
      if (event.type == SDL_EVENT_QUIT) {
        quit = true;
        break;
      }
      dot.handleEvent(event);
    }

    dot.move();

    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    SDL_RenderClear(renderer);

    dot.render();

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