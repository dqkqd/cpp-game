#include <SDL3/SDL.h>
#include <SDL3_image/SDL_image.h>
#include <SDL3_mixer/SDL_mixer.h>
#include <SDL3_ttf/SDL_ttf.h>

#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

#include "SDL_audio.h"
#include "SDL_blendmode.h"
#include "SDL_clipboard.h"
#include "SDL_error.h"
#include "SDL_events.h"
#include "SDL_init.h"
#include "SDL_keyboard.h"
#include "SDL_keycode.h"
#include "SDL_log.h"
#include "SDL_mouse.h"
#include "SDL_pixels.h"
#include "SDL_rect.h"
#include "SDL_render.h"
#include "SDL_rwops.h"
#include "SDL_scancode.h"
#include "SDL_stdinc.h"
#include "SDL_surface.h"
#include "SDL_timer.h"
#include "SDL_video.h"

// Screen dimension constants
const int SCREEN_WIDTH = 640;
const int SCREEN_HEIGHT = 480;

// The dimensions of the level
const int LEVEL_WIDTH = 1280;
const int LEVEL_HEIGHT = 960;

class LTexture {
 public:
  LTexture();
  ~LTexture();
  void free();

  void setColor(Uint8 r, Uint8 g, Uint8 b);
  void setBlendMode(SDL_BlendMode blending);
  void setAlpha(uint8_t alpha);

  bool loadFromFile(std::string path);
  bool loadPixelsFromFile(std::string path);
  bool loadFromPixels();
  bool loadFromRenderedText(std::string textureText, SDL_Color textColor);

  int getWidth();
  int getHeight();

  void render(int x, int y, SDL_FRect* clip = NULL, double angle = 0.0,
              SDL_FPoint* center = NULL, SDL_RendererFlip flip = SDL_FLIP_NONE);

  Uint32* getPixels32();
  Uint32 getPitch32();

 private:
  SDL_Texture* texture_;
  SDL_Surface* surfacePixels_;
  int width_;
  int height_;
};

class Tile {
 public:
  Tile(int x, int y, int tileType);
  void render(SDL_FRect& camera);
  int getType();
  SDL_FRect getBox();

 private:
  SDL_FRect box_;
  int type_;
};

bool init();
void close();
bool loadMedia();
void gameLoop();

SDL_Window* window = NULL;
SDL_Renderer* renderer = NULL;
TTF_Font* font = NULL;

LTexture fooTexture;
LTexture dotTexture;

LTexture::LTexture()
    : texture_(NULL), width_(0), height_(0), surfacePixels_(NULL) {}
LTexture::~LTexture() { free(); }
void LTexture::free() {
  if (texture_ != NULL) {
    SDL_DestroyTexture(texture_);
    texture_ = NULL;
    width_ = 0;
    height_ = 0;
  }

  if (surfacePixels_ != NULL) {
    SDL_DestroySurface(surfacePixels_);
    surfacePixels_ = NULL;
  }
}
void LTexture::setColor(Uint8 r, Uint8 g, Uint8 b) {
  if (SDL_SetTextureColorMod(texture_, r, g, b) < 0) {
    SDL_Log("%s", SDL_GetError());
  }
}
void LTexture::setBlendMode(SDL_BlendMode blending) {
  SDL_SetTextureBlendMode(texture_, blending);
}
void LTexture::setAlpha(uint8_t alpha) {
  SDL_SetTextureAlphaMod(texture_, alpha);
}
bool LTexture::loadFromFile(std::string path) {
  if (loadPixelsFromFile(path)) {
    loadFromPixels();
  }
  return texture_ != NULL;
}
bool LTexture::loadPixelsFromFile(std::string path) {
  free();
  auto surface = IMG_Load(path.c_str());
  if (surface == NULL) {
    SDL_Log("Unable to load image %s, error: %s", path.c_str(), IMG_GetError());
  } else {
    surfacePixels_ =
        SDL_ConvertSurfaceFormat(surface, SDL_GetWindowPixelFormat(window));
    if (surfacePixels_ == NULL) {
      SDL_Log("Unable to convert surface image %s, error: %s", path.c_str(),
              IMG_GetError());
    } else {
      width_ = surfacePixels_->w;
      height_ = surfacePixels_->h;
    }

    SDL_DestroySurface(surface);
  }
  return surfacePixels_ != NULL;
}
bool LTexture::loadFromPixels() {
  if (surfacePixels_ == NULL) {
    printf("No pixel loaded\n");
  } else {
    SDL_SetSurfaceColorKey(surfacePixels_, SDL_TRUE,
                           SDL_MapRGB(surfacePixels_->format, 0, 255, 255));
    texture_ = SDL_CreateTextureFromSurface(renderer, surfacePixels_);
    if (!texture_) {
      SDL_Log("Unable to create texture from loaded pixel %s", SDL_GetError());
    } else {
      width_ = surfacePixels_->w;
      height_ = surfacePixels_->h;
    }
    SDL_DestroySurface(surfacePixels_);
    surfacePixels_ = NULL;
  }
  return texture_ != NULL;
}
bool LTexture::loadFromRenderedText(std::string textureText,
                                    SDL_Color textColor) {
  free();
  auto textSurface = TTF_RenderText_Solid(font, textureText.c_str(), textColor);
  if (textSurface == NULL) {
    SDL_Log("Could not load text surface: %s", TTF_GetError());
  } else {
    texture_ = SDL_CreateTextureFromSurface(renderer, textSurface);
    if (texture_ == NULL) {
      SDL_Log("Could not create texture from surface: %s", SDL_GetError());
    } else {
      width_ = textSurface->w;
      height_ = textSurface->h;
    }
    SDL_DestroySurface(textSurface);
  }
  return texture_ != NULL;
}
int LTexture::getWidth() { return width_; }
int LTexture::getHeight() { return height_; }
void LTexture::render(int x, int y, SDL_FRect* clip, double angle,
                      SDL_FPoint* center, SDL_RendererFlip flip) {
  SDL_FRect viewport{float(x), float(y), float(width_), float(height_)};
  if (clip != NULL) {
    viewport.w = clip->w;
    viewport.h = clip->h;
  }
  SDL_RenderTextureRotated(renderer, texture_, clip, &viewport, angle, center,
                           flip);
}
Uint32* LTexture::getPixels32() {
  Uint32* pixels = NULL;
  if (surfacePixels_ != NULL) {
    pixels = static_cast<Uint32*>(surfacePixels_->pixels);
  }
  return pixels;
}
Uint32 LTexture::getPitch32() {
  Uint32 pitch = 0;
  if (surfacePixels_ != NULL) {
    pitch = surfacePixels_->pitch / 4;
  }
  return pitch;
}

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
      if (!SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "1")) {
        printf("Warning: Linear texture filtering not enabled!");
      }

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
  if (!fooTexture.loadPixelsFromFile(
          "assets/40_texture_manipulation/foo.png")) {
    success = false;
  } else {
    auto pixels = fooTexture.getPixels32();
    int pixelCount = fooTexture.getPitch32() * fooTexture.getHeight();
    auto format = SDL_CreatePixelFormat(SDL_GetWindowPixelFormat(window));

    auto colorKey = SDL_MapRGBA(format, 0xFF, 0x00, 0xFF, 0xFF);
    auto transparent = SDL_MapRGBA(format, 255, 255, 255, 0);

    for (int i = 0; i < pixelCount; ++i) {
      if (pixels[i] == colorKey) {
        pixels[i] = transparent;
      }
    }

    if (!fooTexture.loadFromPixels()) {
      printf("Unable to load texture from surface\n");
    }
  }

  return success;
}

void gameLoop() {
  SDL_Event event;

  bool quit = false;

  bool renderText = false;

  SDL_Color textColor{0, 0, 0, 255};

  while (!quit) {
    while (SDL_PollEvent(&event) != 0) {
      if (event.type == SDL_EVENT_QUIT) {
        quit = true;
        break;
      }
    }

    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    SDL_RenderClear(renderer);
    fooTexture.render(0, 0);
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