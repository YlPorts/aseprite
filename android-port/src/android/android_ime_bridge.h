#pragma once
// Bridge only: LAF requests an Android input method, but Aseprite remains
// the editor processing all text. No parallel text editor or UI is created.
namespace aseandroid {
void requestOriginalEditorIme(bool enabled);
}
