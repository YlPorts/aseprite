#include "os/android/key_state.h"
#include <array>
#include <atomic>
namespace os {
namespace {
std::array<std::atomic_bool,static_cast<int>(kKeyScancodes)>& pressedKeys() {
  static std::array<std::atomic_bool,static_cast<int>(kKeyScancodes)> keys{};
  return keys;
}
}
void androidSetKeyPressed(KeyScancode code,bool pressed) {
  if (code>kKeyNil && code<kKeyScancodes)
    pressedKeys()[static_cast<int>(code)].store(pressed,std::memory_order_release);
}
bool androidIsKeyPressed(KeyScancode code) {
  return code>kKeyNil && code<kKeyScancodes &&
    pressedKeys()[static_cast<int>(code)].load(std::memory_order_acquire);
}
void androidClearPressedKeys() {
  for (auto& key: pressedKeys()) key.store(false,std::memory_order_release);
}
KeyScancode androidMapKey(int code) {
  if (code>=29 && code<=54)
    return static_cast<KeyScancode>(static_cast<int>(kKeyA)+code-29);
  if (code>=7 && code<=16)
    return static_cast<KeyScancode>(static_cast<int>(kKey0)+code-7);
  switch (code) {
    case 4: case 111: return kKeyEsc;
    case 66: return kKeyEnter;
    case 67: return kKeyBackspace;
    case 112: return kKeyDel;
    case 62: return kKeySpace;
    case 61: return kKeyTab;
    case 19: return kKeyUp;
    case 20: return kKeyDown;
    case 21: return kKeyLeft;
    case 22: return kKeyRight;
    case 59: return kKeyLShift;
    case 60: return kKeyRShift;
    case 113: return kKeyLControl;
    case 114: return kKeyRControl;
    case 57: return kKeyAlt;
    case 58: return kKeyAltGr;
    default: return kKeyNil;
  }
}
}
