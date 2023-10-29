#include <SDL3/SDL.h>
#include <SDL3_image/SDL_image.h>
#include <SDL3_mixer/SDL_mixer.h>
#include <SDL3_ttf/SDL_ttf.h>

#include <cstddef>
#include <cstdint>
#include <sstream>
#include <string>
#include <vector>

#include "SDL_blendmode.h"
#include "SDL_error.h"
#include "SDL_events.h"
#include "SDL_init.h"
#include "SDL_keyboard.h"
#include "SDL_keycode.h"
#include "SDL_log.h"
#include "SDL_pixels.h"
#include "SDL_rect.h"
#include "SDL_render.h"
#include "SDL_scancode.h"
#include "SDL_timer.h"

bool init();
void close();
bool loadMedia();
void gameLoop();
bool checkCollision(std::vector<SDL_FRect>& a, std::vector<SDL_FRect>& b);

class LTexture {
 public:
  LTexture() = default;
  ~LTexture();
  void free();
  void setColor(Uint8 r, Uint8 g, Uint8 b);
  void setBlendMode(SDL_BlendMode blending);
  void setAlpha(uint8_t alpha);

  bool loadFromFile(std::string path);
  bool loadFromRenderedText(std::string textureText, SDL_Color textColor);

  int getWidth();
  int getHeight();

  void render(int x, int y, SDL_FRect* clip = NULL, double angle = 0.0,
              SDL_FPoint* center = NULL, SDL_RendererFlip flip = SDL_FLIP_NONE);

 private:
  SDL_Texture* texture_;
  int width_;
  int height_;
};

class LTimer {
 public:
  LTimer();

  void start();
  void stop();
  void pause();
  void unpause();
  uint32_t getTicks();
  bool isStarted();
  bool isPaused();

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
  static constexpr int DOT_VEL = 1;

  Dot(int x, int y);
  void handleEvent(SDL_Event& e);
  void move(std::vector<SDL_FRect>& otherColliders);
  void render();
  std::vector<SDL_FRect>& getColliders();

 private:
  int posX_;
  int posY_;
  int velX_;
  int velY_;

  std::vector<SDL_FRect> colliders_;
  void shiftColliders();
};

constexpr int SCREEN_WIDTH = 640;
constexpr int SCREEN_HEIGHT = 480;
constexpr int SCREEN_FPS = 60;
constexpr int SCREEN_TICKS_PER_FRAME = 1000 / SCREEN_FPS;

SDL_Window* window = NULL;
SDL_Renderer* renderer = NULL;
TTF_Font* font = NULL;

LTexture dotTexture;
SDL_AudioSpec spec;

LTexture::~LTexture() { free(); }
void LTexture::free() {
  if (texture_ != NULL) {
    SDL_DestroyTexture(texture_);
    texture_ = NULL;
    width_ = 0;
    height_ = 0;
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
  free();
  texture_ = IMG_LoadTexture(renderer, path.c_str());
  if (texture_ == NULL) {
    SDL_Log("%s", IMG_GetError());
  } else if (SDL_QueryTexture(texture_, NULL, NULL, &width_, &height_) < 0) {
    SDL_Log("%s", SDL_GetError());
  }
  return true;
}
bool LTexture::loadFromRenderedText(std::string textureText,
                                    SDL_Color textColor) {
  free();
  auto textSurface = TTF_RenderText_Solid(font, textureText.c_str(), textColor);
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

LTimer::LTimer()
    : started_(false), paused_(false), startTicks_(0), pausedTicks_(0) {}

void LTimer::start() {
  started_ = true;
  paused_ = false;

  startTicks_ = SDL_GetTicks();
  pausedTicks_ = 0;
}
void LTimer::stop() {
  started_ = false;
  paused_ = false;

  startTicks_ = 0;
  pausedTicks_ = 0;
}
void LTimer::pause() {
  if (started_ && !paused_) {
    paused_ = true;
    pausedTicks_ = SDL_GetTicks() - startTicks_;
    startTicks_ = 0;
  }
}
void LTimer::unpause() {
  if (started_ && paused_) {
    paused_ = false;
    startTicks_ = SDL_GetTicks() - pausedTicks_;
    pausedTicks_ = 0;
  }
}
uint32_t LTimer::getTicks() {
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
bool LTimer::isStarted() { return started_; }
bool LTimer::isPaused() { return paused_; }

Dot::Dot(int x, int y) : posX_(x), posY_(y), velX_(0), velY_(0) {
  colliders_.resize(11);

  colliders_[0].w = 6;
  colliders_[0].h = 1;

  colliders_[1].w = 10;
  colliders_[1].h = 1;

  colliders_[2].w = 14;
  colliders_[2].h = 1;

  colliders_[3].w = 16;
  colliders_[3].h = 2;

  colliders_[4].w = 18;
  colliders_[4].h = 2;

  colliders_[5].w = 20;
  colliders_[5].h = 6;

  colliders_[6].w = 18;
  colliders_[6].h = 2;

  colliders_[7].w = 16;
  colliders_[7].h = 2;

  colliders_[8].w = 14;
  colliders_[8].h = 1;

  colliders_[9].w = 10;
  colliders_[9].h = 1;

  colliders_[10].w = 6;
  colliders_[10].h = 1;

  shiftColliders();
}
void Dot::handleEvent(SDL_Event& e) {
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
void Dot::move(std::vector<SDL_FRect>& otherColliders) {
  posX_ += velX_;
  shiftColliders();

  if (posX_ < 0 || posX_ + DOT_WIDTH > SCREEN_WIDTH ||
      checkCollision(colliders_, otherColliders)) {
    posX_ -= velX_;
    shiftColliders();
  }

  posY_ += velY_;
  shiftColliders();
  if (posY_ < 0 || posY_ + DOT_HEIGHT > SCREEN_HEIGHT ||
      checkCollision(colliders_, otherColliders)) {
    posY_ -= velY_;
    shiftColliders();
  }
}
void Dot::render() { dotTexture.render(posX_, posY_); }
void Dot::shiftColliders() {
  int r = 0;
  for (int set = 0; set < colliders_.size(); ++set) {
    colliders_[set].x = posX_ + (DOT_WIDTH - colliders_[set].w) / 2;
    colliders_[set].y = posY_ + r;
    r += colliders_[set].h;
  }
}
std::vector<SDL_FRect>& Dot::getColliders() { return colliders_; }

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

bool checkCollision(std::vector<SDL_FRect>& a, std::vector<SDL_FRect>& b) {
  int leftA, leftB;
  int rightA, rightB;
  int topA, topB;
  int bottomA, bottomB;

  // Go through the A boxes
  for (int aBox = 0; aBox < a.size(); aBox++) {
    // Calculate the sides of rect A
    leftA = a[aBox].x;
    rightA = a[aBox].x + a[aBox].w;
    topA = a[aBox].y;
    bottomA = a[aBox].y + a[aBox].h;

    // Go through the B boxes
    for (int Bbox = 0; Bbox < b.size(); Bbox++) {
      // Calculate the sides of rect B
      leftB = b[Bbox].x;
      rightB = b[Bbox].x + b[Bbox].w;
      topB = b[Bbox].y;
      bottomB = b[Bbox].y + b[Bbox].h;

      // If no sides from A are outside of B
      if (((bottomA <= topB) || (topA >= bottomB) || (rightA <= leftB) ||
           (leftA >= rightB)) == false) {
        // A collision is detected
        return true;
      }
    }
  }

  return false;
}

void gameLoop() {
  SDL_Event event;

  bool quit = false;
  Dot dot(0, 0);
  Dot otherDot(SCREEN_WIDTH / 4, SCREEN_HEIGHT / 4);

  SDL_FRect wall{300, 40, 40, 400};

  while (!quit) {
    while (SDL_PollEvent(&event) != 0) {
      if (event.type == SDL_EVENT_QUIT) {
        quit = true;
        break;
      }
      dot.handleEvent(event);
    }

    dot.move(otherDot.getColliders());

    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    SDL_RenderClear(renderer);

    dot.render();
    otherDot.render();

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