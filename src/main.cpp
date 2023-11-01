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
#include "SDL_timer.h"

// Screen dimension constants
const int SCREEN_WIDTH = 640;
const int SCREEN_HEIGHT = 480;

// The dimensions of the level
const int LEVEL_WIDTH = 1280;
const int LEVEL_HEIGHT = 960;

// Tile constants
const int TILE_WIDTH = 80;
const int TILE_HEIGHT = 80;
const int TOTAL_TILES = 192;
const int TOTAL_TILE_SPRITES = 12;

// The different tile sprites
const int TILE_RED = 0;
const int TILE_GREEN = 1;
const int TILE_BLUE = 2;
const int TILE_CENTER = 3;
const int TILE_TOP = 4;
const int TILE_TOPRIGHT = 5;
const int TILE_RIGHT = 6;
const int TILE_BOTTOMRIGHT = 7;
const int TILE_BOTTOM = 8;
const int TILE_BOTTOMLEFT = 9;
const int TILE_LEFT = 10;
const int TILE_TOPLEFT = 11;

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

class Dot {
 public:
  static constexpr int DOT_WIDTH = 20;
  static constexpr int DOT_HEIGHT = 20;
  static constexpr int DOT_VEL = 10;

  Dot(int x = 0, int y = 0);
  void handleEvent(SDL_Event& e);
  void move(Tile* tiles[]);
  void setCamera(SDL_FRect& camera);
  void render(SDL_FRect& camera);

 private:
  SDL_FRect box_;
  int velX_;
  int velY_;
};

bool init();
void close(Tile* tiles[]);
bool loadMedia(Tile* tiles[]);
void gameLoop(Tile* tiles[]);
bool checkCollision(SDL_FRect a, SDL_FRect b);
bool touchesWall(SDL_FRect box, Tile* tiles[]);
bool setTiles(Tile* tiles[]);

SDL_Window* window = NULL;
SDL_Renderer* renderer = NULL;
TTF_Font* font = NULL;

SDL_FRect tileClips[TOTAL_TILE_SPRITES];
LTexture tileTexture;

LTexture dotTexture;

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

Tile::Tile(int x, int y, int tileType)
    : box_{float(x), float(y), TILE_WIDTH, TILE_HEIGHT}, type_(tileType) {}
void Tile::render(SDL_FRect& camera) {
  if (checkCollision(camera, box_)) {
    tileTexture.render(box_.x - camera.x, box_.y - camera.y, &tileClips[type_]);
  }
}
int Tile::getType() { return type_; }
SDL_FRect Tile::getBox() { return box_; }

Dot::Dot(int x, int y)
    : box_{0, 0, DOT_WIDTH, DOT_HEIGHT}, velX_(0), velY_(0) {}
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
void Dot::move(Tile* tiles[]) {
  box_.x += velX_;
  if (box_.x < 0 || box_.x + DOT_WIDTH > LEVEL_WIDTH ||
      touchesWall(box_, tiles)) {
    box_.x -= velX_;
  }

  box_.y += velY_;
  if (box_.y < 0 || box_.y + DOT_HEIGHT > SCREEN_HEIGHT ||
      touchesWall(box_, tiles)) {
    box_.y -= velY_;
  }
}
void Dot::setCamera(SDL_FRect& camera) {
  camera.x = (box_.x + DOT_WIDTH / 2.0) - SCREEN_WIDTH / 2.0;
  camera.y = (box_.y + DOT_HEIGHT / 2.0) - SCREEN_HEIGHT / 2.0;

  if (camera.x < 0) {
    camera.x = 0;
  }
  if (camera.y < 0) {
    camera.y = 0;
  }
  if (camera.x > LEVEL_WIDTH - camera.w) {
    camera.x = LEVEL_WIDTH - camera.w;
  }
  if (camera.y > LEVEL_HEIGHT - camera.h) {
    camera.y = LEVEL_HEIGHT - camera.h;
  }
}
void Dot::render(SDL_FRect& camera) {
  dotTexture.render(box_.x - camera.x, box_.y - camera.y);
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

bool loadMedia(Tile* tiles[]) {
  bool success = true;

  if (!dotTexture.loadFromFile("assets/39_tiling/dot.bmp")) {
    printf("Failed to load dot file\n");
    success = false;
  }
  if (!tileTexture.loadFromFile("assets/39_tiling/tiles.png")) {
    printf("Failed to load tile file\n");
    success = false;
  }

  if (!setTiles(tiles)) {
    printf("Failed to load tile set\n");
    success = false;
  }

  return success;
}

bool checkCollision(SDL_FRect a, SDL_FRect b) {
  // The sides of the rectangles
  int leftA, leftB;
  int rightA, rightB;
  int topA, topB;
  int bottomA, bottomB;

  // Calculate the sides of rect A
  leftA = a.x;
  rightA = a.x + a.w;
  topA = a.y;
  bottomA = a.y + a.h;

  // Calculate the sides of rect B
  leftB = b.x;
  rightB = b.x + b.w;
  topB = b.y;
  bottomB = b.y + b.h;

  // If any of the sides from A are outside of B
  if (bottomA <= topB) {
    return false;
  }

  if (topA >= bottomB) {
    return false;
  }

  if (rightA <= leftB) {
    return false;
  }

  if (leftA >= rightB) {
    return false;
  }

  // If none of the sides from A are outside B
  return true;
}

bool setTiles(Tile* tiles[]) {
  bool tilesLoaded = true;
  int x = 0;
  int y = 0;

  std::ifstream map("assets/39_tiling/lazy.map");
  if (map.fail()) {
    printf("Unable to load map file\n");
    tilesLoaded = false;
  } else {
    for (int i = 0; i < TOTAL_TILES; ++i) {
      int tileType = -1;
      map >> tileType;
      if (map.fail()) {
        printf("Error loading map\n");
        tilesLoaded = false;
        break;
      }

      if (tileType >= 0 && tileType < TOTAL_TILE_SPRITES) {
        tiles[i] = new Tile(x, y, tileType);
      } else {
        printf("Error loading map: invalid tile type\n");
        tilesLoaded = false;
        break;
      }

      x += TILE_WIDTH;
      if (x >= LEVEL_WIDTH) {
        x = 0;
        y += TILE_HEIGHT;
      }
    }
    if (tilesLoaded) {
      tileClips[TILE_RED].x = 0;
      tileClips[TILE_RED].y = 0;
      tileClips[TILE_RED].w = TILE_WIDTH;
      tileClips[TILE_RED].h = TILE_HEIGHT;

      tileClips[TILE_GREEN].x = 0;
      tileClips[TILE_GREEN].y = 80;
      tileClips[TILE_GREEN].w = TILE_WIDTH;
      tileClips[TILE_GREEN].h = TILE_HEIGHT;

      tileClips[TILE_BLUE].x = 0;
      tileClips[TILE_BLUE].y = 160;
      tileClips[TILE_BLUE].w = TILE_WIDTH;
      tileClips[TILE_BLUE].h = TILE_HEIGHT;

      tileClips[TILE_TOPLEFT].x = 80;
      tileClips[TILE_TOPLEFT].y = 0;
      tileClips[TILE_TOPLEFT].w = TILE_WIDTH;
      tileClips[TILE_TOPLEFT].h = TILE_HEIGHT;

      tileClips[TILE_LEFT].x = 80;
      tileClips[TILE_LEFT].y = 80;
      tileClips[TILE_LEFT].w = TILE_WIDTH;
      tileClips[TILE_LEFT].h = TILE_HEIGHT;

      tileClips[TILE_BOTTOMLEFT].x = 80;
      tileClips[TILE_BOTTOMLEFT].y = 160;
      tileClips[TILE_BOTTOMLEFT].w = TILE_WIDTH;
      tileClips[TILE_BOTTOMLEFT].h = TILE_HEIGHT;

      tileClips[TILE_TOP].x = 160;
      tileClips[TILE_TOP].y = 0;
      tileClips[TILE_TOP].w = TILE_WIDTH;
      tileClips[TILE_TOP].h = TILE_HEIGHT;

      tileClips[TILE_CENTER].x = 160;
      tileClips[TILE_CENTER].y = 80;
      tileClips[TILE_CENTER].w = TILE_WIDTH;
      tileClips[TILE_CENTER].h = TILE_HEIGHT;

      tileClips[TILE_BOTTOM].x = 160;
      tileClips[TILE_BOTTOM].y = 160;
      tileClips[TILE_BOTTOM].w = TILE_WIDTH;
      tileClips[TILE_BOTTOM].h = TILE_HEIGHT;

      tileClips[TILE_TOPRIGHT].x = 240;
      tileClips[TILE_TOPRIGHT].y = 0;
      tileClips[TILE_TOPRIGHT].w = TILE_WIDTH;
      tileClips[TILE_TOPRIGHT].h = TILE_HEIGHT;

      tileClips[TILE_RIGHT].x = 240;
      tileClips[TILE_RIGHT].y = 80;
      tileClips[TILE_RIGHT].w = TILE_WIDTH;
      tileClips[TILE_RIGHT].h = TILE_HEIGHT;

      tileClips[TILE_BOTTOMRIGHT].x = 240;
      tileClips[TILE_BOTTOMRIGHT].y = 160;
      tileClips[TILE_BOTTOMRIGHT].w = TILE_WIDTH;
      tileClips[TILE_BOTTOMRIGHT].h = TILE_HEIGHT;
    }
  }
  map.close();
  return tilesLoaded;
}

bool touchesWall(SDL_FRect box, Tile* tiles[]) {
  for (int i = 0; i < TOTAL_TILES; ++i) {
    if (tiles[i]->getType() >= TILE_CENTER &&
        tiles[i]->getType() <= TILE_TOPLEFT) {
      if (checkCollision(box, tiles[i]->getBox())) {
        return true;
      }
    }
  }
  return false;
}

void gameLoop(Tile* tiles[]) {
  SDL_Event event;

  bool quit = false;

  bool renderText = false;

  SDL_Color textColor{0, 0, 0, 255};
  Dot dot;
  SDL_FRect camera = {0, 0, SCREEN_WIDTH, SCREEN_HEIGHT};

  while (!quit) {
    while (SDL_PollEvent(&event) != 0) {
      if (event.type == SDL_EVENT_QUIT) {
        quit = true;
        break;
      }
      dot.handleEvent(event);
    }
    dot.move(tiles);
    dot.setCamera(camera);

    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    SDL_RenderClear(renderer);
    for (int i = 0; i < TOTAL_TILES; ++i) {
      tiles[i]->render(camera);
    }
    dot.render(camera);
    SDL_RenderPresent(renderer);

    if (quit) {
      break;
    }
  }
}

int main(int argc, char* argv[]) {
  init();

  Tile* tileSet[TOTAL_TILES];
  if (loadMedia(tileSet)) {
    gameLoop(tileSet);
  }
  close();
  return 0;
}