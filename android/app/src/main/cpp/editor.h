#pragma once
#include "doc/sprite.h"
#include <cstdint>
#include <memory>
#include <vector>
#include <string>
namespace mobile {
class Editor {
public:
  Editor(int width, int height);
  ~Editor();
  std::vector<int> info() const;
  std::vector<uint32_t> render();
  std::vector<uint8_t> save();
  void open(const std::vector<uint8_t>& bytes);
  void beginStroke();
  void line(int x0, int y0, int x1, int y1, uint32_t argb, int size, bool erase);
  bool command(int op, int value);
  std::string layerNames() const;
private:
  std::unique_ptr<doc::Sprite> sprite_;
  int frame_ = 0, layer_ = 0;
  std::vector<std::vector<uint8_t>> undo_, redo_;
  void checkpoint();
  void restore(const std::vector<uint8_t>& bytes);
};
}
