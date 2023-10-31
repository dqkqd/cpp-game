#include <SDL3/SDL.h>
#include <SDL3_image/SDL_image.h>
#include <SDL3_mixer/SDL_mixer.h>
#include <SDL3_ttf/SDL_ttf.h>

#include <cstddef>
#include <cstdint>
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
#include "SDL_oldnames.h"
#include "SDL_pixels.h"
#include "SDL_rect.h"
#include "SDL_render.h"
#include "SDL_rwops.h"
#include "SDL_scancode.h"
#include "SDL_stdinc.h"
#include "SDL_timer.h"
#include "SDL_video.h"

struct Circle {
  int x, y;
  int r;
};

bool init();
void close();
bool loadMedia();
void gameLoop();
bool checkCollision(Circle& a, Circle& b);
bool checkCollision(Circle& a, SDL_FRect& b);
double distanceSquared(int x1, int y1, int x2, int y2);

constexpr int MAX_RECORDING_DEVICES = 10;
constexpr int MAX_RECORDING_SECONDS = 5;
constexpr int RECORDING_BUFFER_SECONDS = MAX_RECORDING_SECONDS + 1;
enum class RecordingState { STOPPED, RECORDING, PLAYBACK };

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
  static constexpr int DOT_VEL = 10;

  Dot(int x = 0, int y = 0);
  void handleEvent(SDL_Event& e);
  void move();
  void render();
  int getPosX();
  int getPosY();

 private:
  int posX_;
  int posY_;
  int velX_;
  int velY_;
};

class LWindow {
 public:
  LWindow();
  bool init();
  SDL_Renderer* createRenderer();
  void handleEvent(SDL_Event& e);

  void focus();
  void render();
  void free();

  int getWidth();
  int getHeight();

  bool hasMouseFocus();
  bool hasKeyboardFocus();
  bool isMinimized();
  bool isShown();

 private:
  SDL_Window* window_;
  SDL_Renderer* renderer_;
  int windowId_;
  int windowDisplayId_;

  int width_;
  int height_;

  bool mouseFocus_;
  bool keyboardFocus_;
  bool fullScreen_;
  bool minimized_;
  bool shown_;
};

constexpr int LEVEL_WIDTH = 1280;
constexpr int LEVEL_HEIGHT = 960;
constexpr int SCREEN_WIDTH = 640;
constexpr int SCREEN_HEIGHT = 480;
constexpr int TOTAL_DATA = 100;
constexpr int SCREEN_FPS = 60;
constexpr int SCREEN_TICKS_PER_FRAME = 1000 / SCREEN_FPS;

LWindow window;
int totalDisplays;
SDL_DisplayID* displays;
SDL_Rect* displayBounds = NULL;

SDL_Renderer* renderer;
LTexture screenTexture;

TTF_Font* font = NULL;

LTexture promptTexture;

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

Dot::Dot(int x, int y) : posX_(x), posY_(y), velX_(0), velY_(0) {}
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
void Dot::move() {
  posX_ += velX_;
  if (posX_ < 0 || posX_ + DOT_WIDTH > SCREEN_WIDTH) {
    posX_ -= velX_;
  }

  posY_ += velY_;
  if (posY_ < 0 || posY_ + DOT_HEIGHT > SCREEN_HEIGHT) {
    posY_ -= velY_;
  }
}
void Dot::render() { promptTexture.render(posX_, posY_); }
int Dot::getPosX() { return posX_; }
int Dot::getPosY() { return posY_; }

LWindow::LWindow()
    : window_(NULL),
      mouseFocus_(false),
      keyboardFocus_(false),
      fullScreen_(false),
      minimized_(false),
      width_(0),
      height_(0) {}

bool LWindow::init() {
  window_ = SDL_CreateWindow("Hello SDL", SCREEN_WIDTH, SCREEN_HEIGHT,
                             SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);
  if (window_) {
    mouseFocus_ = true;
    keyboardFocus_ = true;
    width_ = SCREEN_WIDTH;
    height_ = SCREEN_HEIGHT;

    windowId_ = SDL_GetWindowID(window_);
    windowDisplayId_ = SDL_GetDisplayForWindow(window_);
    shown_ = true;

    renderer_ = SDL_CreateRenderer(
        window_, NULL, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (renderer_) {
      SDL_SetRenderDrawColor(renderer_, 255, 255, 255, 255);
    }
  }
  return window_ != NULL && renderer_ != NULL;
}

void LWindow::handleEvent(SDL_Event& e) {
  if (e.type >= SDL_EVENT_WINDOW_FIRST && e.type <= SDL_EVENT_WINDOW_LAST &&
      e.window.windowID == windowId_) {
    bool updateCaption = false;
    switch (e.window.type) {
      case SDL_EVENT_WINDOW_MOVED:
        windowDisplayId_ = SDL_GetDisplayForWindow(window_);
        updateCaption = true;
        break;
      case SDL_EVENT_WINDOW_SHOWN:
        shown_ = true;
        break;
      case SDL_EVENT_WINDOW_HIDDEN:
        shown_ = false;
        break;
      case SDL_EVENT_WINDOW_CLOSE_REQUESTED:
        SDL_HideWindow(window_);
        break;
      case SDL_EVENT_WINDOW_RESIZED:
        width_ = e.window.data1;
        height_ = e.window.data2;
        SDL_RenderPresent(renderer);
        break;
      case SDL_EVENT_WINDOW_EXPOSED:
        SDL_RenderPresent(renderer);
        break;
      case SDL_EVENT_WINDOW_MOUSE_ENTER:
        mouseFocus_ = true;
        updateCaption = true;
        break;
      case SDL_EVENT_WINDOW_MOUSE_LEAVE:
        mouseFocus_ = false;
        updateCaption = true;
        break;
      case SDL_EVENT_WINDOW_FOCUS_GAINED:
        keyboardFocus_ = true;
        updateCaption = true;
        break;
      case SDL_EVENT_WINDOW_FOCUS_LOST:
        keyboardFocus_ = false;
        updateCaption = true;
        break;
      case SDL_EVENT_WINDOW_MINIMIZED:
        minimized_ = true;
        break;
      case SDL_EVENT_WINDOW_MAXIMIZED:
        minimized_ = false;
        break;
      case SDL_EVENT_WINDOW_RESTORED:
        minimized_ = false;
        break;
    }
    if (updateCaption) {
      std::stringstream caption;
      caption << "Hello SDL - MouseFocus:" << ((mouseFocus_) ? "On" : "Off")
              << " KeyboardFocus:" << ((keyboardFocus_) ? "On" : "Off");
      SDL_SetWindowTitle(window_, caption.str().c_str());
    }
  } else if ((e.type = SDL_EVENT_KEY_DOWN)) {
    bool switchDisplay = false;
    switch (e.key.keysym.sym) {
      case SDLK_UP:
        ++windowDisplayId_;
        switchDisplay = true;
        break;
      case SDLK_DOWN:
        --windowDisplayId_;
        switchDisplay = true;
        break;
    }
    if (switchDisplay) {
      if (windowDisplayId_ < 1) {
        printf("-\n");
        windowDisplayId_ = totalDisplays;
      } else if (windowDisplayId_ >= totalDisplays + 1) {
        printf("+\n");
        windowDisplayId_ = 1;
      }

      windowDisplayId_ = 1;
      printf("Switch %d\n", windowDisplayId_);
      for (int i = 0; i < totalDisplays; ++i) {
        printf("%d %d %d %d\n", displayBounds[i].x, displayBounds[i].y,
               displayBounds[i].w, displayBounds[i].h);
      }
      SDL_SetWindowPosition(window_, 0, 0);
    }
  }
}
void LWindow::focus() {
  if (!shown_) {
    SDL_ShowWindow(window_);
  }
  SDL_RaiseWindow(window_);
}
void LWindow::render() {
  if (!minimized_) {
    SDL_SetRenderDrawColor(renderer_, 255, 255, 255, 255);
    SDL_RenderClear(renderer_);
    SDL_RenderPresent(renderer_);
  }
}
int LWindow::getWidth() { return width_; }
int LWindow::getHeight() { return height_; }
bool LWindow::hasMouseFocus() { return mouseFocus_; }
bool LWindow::hasKeyboardFocus() { return keyboardFocus_; }
bool LWindow::isMinimized() { return minimized_; }
bool LWindow::isShown() { return shown_; }
void LWindow::free() { SDL_DestroyWindow(window_); }

bool init() {
  bool success = true;

  if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) < 0) {
    SDL_Log("SDL_Init failed (%s)", SDL_GetError());
    success = false;
  } else {
    if (!window.init()) {
      SDL_Log("Could not init window %s", SDL_GetError());
      success = false;
    } else {
      if (!SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "1")) {
        printf("Warning: Linear texture filtering not enabled!");
      }

      displays = SDL_GetDisplays(&totalDisplays);
      if (totalDisplays < 2) {
        printf("Only one displays connected\n");
        success = false;
      }

      displayBounds = new SDL_Rect[totalDisplays];
      for (int i = 0; i < totalDisplays; ++i) {
        SDL_GetDisplayBounds(displays[i], &displayBounds[i]);
      }
    }
  }

  return success;
}

void close() {
  promptTexture.free();

  Mix_CloseAudio();

  TTF_CloseFont(font);
  SDL_DestroyRenderer(renderer);

  window.free();

  Mix_Quit();
  TTF_Quit();
  IMG_Quit();
  SDL_Quit();
}

bool loadMedia() {
  bool success = true;
  return success;
}

double distanceSquared(int x1, int y1, int x2, int y2) {
  return (x1 - x2) * (x1 - x2) + (y1 - y2) * (y1 - y2);
}
bool checkCollision(Circle& a, Circle& b) {
  return distanceSquared(a.x, a.y, b.x, b.y) < (a.r + b.r) * (a.r + b.r);
}
bool checkCollision(Circle& a, SDL_FRect& b) {
  int cX, cY;
  if (a.x < b.x) {
    cX = b.x;
  } else if (a.x > b.x + b.w) {
    cX = b.x + b.w;
  } else {
    cX = a.x;
  }

  if (a.y < b.y) {
    cY = b.y;
  } else if (a.y > b.y + b.h) {
    cY = b.y + b.h;
  } else {
    cY = a.y;
  }

  return distanceSquared(a.x, a.y, cX, cY) < a.r * a.r;
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
      window.handleEvent(event);
    }

    window.render();

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