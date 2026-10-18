#pragma once
#include "os/keys.h"
namespace os {
void androidSetKeyPressed(KeyScancode code,bool pressed);
bool androidIsKeyPressed(KeyScancode code);
void androidClearPressedKeys();
KeyScancode androidMapKey(int androidKeyCode);
}
