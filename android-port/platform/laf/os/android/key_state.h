#pragma once
#include "os/keys.h"
#include "gfx/point.h"
namespace os {
void androidSetKeyPressed(KeyScancode code,bool pressed);
bool androidIsKeyPressed(KeyScancode code);
void androidClearPressedKeys();
KeyScancode androidMapKey(int androidKeyCode);
void androidSetPointerPosition(int x,int y);
gfx::Point androidPointerPosition();
}
