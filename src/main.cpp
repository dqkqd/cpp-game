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

LTexture modulatedTexture;

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

  if (!modulatedTexture.loadFromFile("assets/12_color_modulation/colors.png")) {
    success = false;
  }

  return success;
}

void gameLoop() {

  SDL_Event event;

  bool quit = false;
  uint8_t r = 255;
  uint8_t g = 255;
  uint8_t b = 255;

  while (!quit) {
    while (SDL_PollEvent(&event)) {
      if (event.type == SDL_EVENT_QUIT) {
        quit = true;
        break;
      } else if (event.type == SDL_EVENT_KEY_DOWN) {
        switch (event.key.keysym.sym) {
        case SDLK_q:
          r += 32;
          break;
        case SDLK_w:
          g += 32;
          break;
        case SDLK_e:
          b += 32;
          break;
        case SDLK_a:
          r -= 32;
          break;
        case SDLK_s:
          g -= 32;
          break;
        case SDLK_d:
          b -= 32;
          break;
        }
      }

      SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
      SDL_RenderClear(renderer);
      modulatedTexture.setColor(r, g, b);
      modulatedTexture.render(0, 0);
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