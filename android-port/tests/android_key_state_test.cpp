// Test the Android scancode adapter against ORIGINAL laf/os/keys.h.
// Compiled in the integration-preflight checkout, after the LAF overlay.
#include "os/android/key_state.h"
#include <cassert>
#include <iostream>

int main() {
  using namespace os;
  androidClearPressedKeys();
  assert(!androidIsKeyPressed(kKeyA));
  assert(androidMapKey(29) == kKeyA);     // KEYCODE_A
  assert(androidMapKey(54) == kKeyZ);     // KEYCODE_Z
  assert(androidMapKey(7) == kKey0);      // KEYCODE_0
  assert(androidMapKey(16) == kKey9);     // KEYCODE_9
  assert(androidMapKey(4) == kKeyEsc);    // Android BACK
  assert(androidMapKey(66) == kKeyEnter); // ENTER
  assert(androidMapKey(67) == kKeyBackspace);
  assert(androidMapKey(62) == kKeySpace);
  assert(androidMapKey(19) == kKeyUp);
  assert(androidMapKey(20) == kKeyDown);
  assert(androidMapKey(21) == kKeyLeft);
  assert(androidMapKey(22) == kKeyRight);
  assert(androidMapKey(999) == kKeyNil);

  androidSetKeyPressed(kKeyLShift, true);
  androidSetKeyPressed(kKeySpace, true);
  assert(androidIsKeyPressed(kKeyLShift));
  assert(androidIsKeyPressed(kKeySpace));
  androidSetKeyPressed(kKeySpace, false);
  assert(!androidIsKeyPressed(kKeySpace));
  androidClearPressedKeys();
  assert(!androidIsKeyPressed(kKeyLShift));

  androidSetPointerPosition(321, 123);
  const gfx::Point p = androidPointerPosition();
  assert(p.x == 321 && p.y == 123);
  std::cout << "PASS: original LAF keyboard map, key state, pointer position\n";
}
