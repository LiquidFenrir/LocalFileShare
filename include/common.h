#pragma once

#include <memory>
#include <algorithm>
#include <numeric>
#include <functional>
#include <filesystem>

namespace fs = std::filesystem;

#include <vector>

#include <cstdlib>
#include <cstdio>
#include <cstring>

#include <3ds.h>
#include <citro3d.h>
#include <citro2d.h>

// #define NODEBUG
#ifdef NODEBUG
#define DEBUG(...)  
#else
#define DEBUG(...) fprintf(stderr, __VA_ARGS__)
#endif

#include "sprites.h"

extern bool running;

extern C2D_SpriteSheet spritesheet;

extern C3D_RenderTarget *top, *bottom;
extern C2D_TextBuf staticBuf, dynamicBuf;

extern u32 kDown, kHeld, kUp;
extern touchPosition touch;

static constexpr u32 whiteColor = C2D_Color32f(1, 1, 1, 1);
static constexpr u32 blackColor = C2D_Color32f(0, 0, 0, 1);
static constexpr u32 backgroundColor = C2D_Color32(0x20, 0x20, 0x20, 0xFF);  // Some nice gray, taken from QRaken
static constexpr u32 textColor = whiteColor;
static constexpr u32 selectedTextColor = blackColor;

static constexpr int maxLinesPerScreen = 6; // Bottom screen height divided by icon size is slightly above 7, minus room for arrows = 6

static constexpr float textScale = 0.6f;

#include "helpers.h"
