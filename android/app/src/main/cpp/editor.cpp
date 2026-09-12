#include "editor.h"
#include "doc/cel.h"
#include "doc/cel_data.h"
#include "doc/color.h"
#include "doc/image.h"
#include "doc/layer.h"
#include "doc/frames_sequence.h"
#include "dio/aseprite_encoder.h"
#include "dio/aseprite_decoder.h"
#include "dio/encode_delegate.h"
#include "dio/decode_delegate.h"
#include "dio/file_interface.h"
#include "render/render.h"
#include <algorithm>
#include <cstring>
#include <stdexcept>
#include <cmath>
namespace mobile {
namespace {
constexpr size_t MAX_FILE = 32u * 1024u * 1024u;
constexpr size_t MAX_HISTORY = 48u * 1024u * 1024u;
class MemoryFile : public dio::FileInterface {
public:
  std::vector<uint8_t> bytes;
  size_t pos = 0;
  bool valid = true, writing;
  explicit MemoryFile(bool w) : writing(w) {}
  explicit MemoryFile(const std::vector<uint8_t>& b) : bytes(b), writing(false) {}
  bool ok() const override { return valid; }
  size_t tell() const override { return pos; }
  void seek(size_t p) override {
    if (p > MAX_FILE || (!writing && p > bytes.size())) { valid = false; return; }
    pos = p;
  }
  uint8_t read8() override { uint8_t v = 0; readBytes(&v, 1); return v; }
  size_t readBytes(uint8_t* p, size_t n) override {
    if (pos > bytes.size() || n > bytes.size() - pos) { valid = false; return 0; }
    std::memcpy(p, bytes.data() + pos, n); pos += n; return n;
  }
  void write8(uint8_t b) override { writeBytes(&b, 1); }
  size_t writeBytes(uint8_t* p, size_t n) override {
    if (!writing || pos > MAX_FILE || n > MAX_FILE - pos) throw std::runtime_error("Documento demasiado grande para esta alpha");
    if (pos + n > bytes.size()) bytes.resize(pos + n);
    std::memcpy(bytes.data() + pos, p, n); pos += n; return n;
  }
};
class Writer : public dio::EncodeDelegate {
  doc::Sprite* s_;
  doc::FramesSequence frames_;
public:
  std::string message;
  explicit Writer(doc::Sprite* s) : s_(s) { frames_.insert(0, s->totalFrames()-1); }
  doc::Sprite* sprite() override { return s_; }
  const doc::FramesSequence& framesSequence() const override { return frames_; }
  bool composeGroups() override { return true; }
  bool preserveColorProfile() override { return true; }
  bool cacheCompressedTilesets() override { return false; }
  void error(const std::string& e) override { message += e; }
};
class Reader : public dio::DecodeDelegate {
public:
  std::unique_ptr<doc::Sprite> result;
  std::string message;
  void onSprite(doc::Sprite* s) override { result.reset(s); }
  void error(const std::string& e) override { message += e; }
  void incompatibilityError(const std::string& e) override { throw std::runtime_error("Formato no admitido en esta alpha: " + e); }
};
std::vector<doc::Layer*> editableLayers(doc::Sprite* s) {
  std::vector<doc::Layer*> out;
  for (auto* layer : s->allLayers()) if (layer->isImage() && !layer->isTilemap() && !layer->isReference()) out.push_back(layer);
  return out;
}
uint16_t u16(const std::vector<uint8_t>& b, size_t i) { return b.at(i) | (uint16_t(b.at(i+1)) << 8); }
uint32_t u32(const std::vector<uint8_t>& b, size_t i) { return u16(b,i) | (uint32_t(u16(b,i+2)) << 16); }
void preflight(const std::vector<uint8_t>& b) {
  if (b.size() < 128 || b.size() > MAX_FILE || u16(b,4) != 0xa5e0 || u32(b,0) != b.size()) throw std::runtime_error("Archivo ASEPRITE no valido o demasiado grande");
  if (!u16(b,6) || u16(b,6)>128 || !u16(b,8) || !u16(b,10) || u16(b,8)>1024 || u16(b,10)>1024) throw std::runtime_error("Limite alpha: 1024x1024 y 128 fotogramas");
  if (u16(b,12) != 32) throw std::runtime_error("Esta alpha edita archivos RGBA; los modos indexado y gris estan pendientes");
  // Inspect chunk bounds before the upstream decoder can allocate cel images.
  size_t pos = 128, decoded = 0;
  for (unsigned f = 0; f < u16(b,6); ++f) {
    if (pos+16 > b.size() || u16(b,pos+4)!=0xf1fa) throw std::runtime_error("Fotograma invalido");
    const size_t frameSize=u32(b,pos), end=pos+frameSize;
    if (frameSize<16 || end>b.size()) throw std::runtime_error("Fotograma truncado");
    size_t p=pos+16;
    while(p<end) {
      if(p+6>end) throw std::runtime_error("Chunk truncado");
      size_t len=u32(b,p); unsigned type=u16(b,p+4);
      if(len<6 || len>end-p) throw std::runtime_error("Tamano de chunk invalido");
      if(type==0x2023) throw std::runtime_error("Los tilesets no se editan todavia en esta alpha");
      if(type==0x2004 && len>=22 && (u16(b,p+6)&64)) throw std::runtime_error("Las capas de referencia no se editan todavia");
      if(type==0x2005) {
        if(len<22) throw std::runtime_error("Cel invalido");
        unsigned kind=u16(b,p+13);
        if(kind==0 || kind==2) {
          if(len<26) throw std::runtime_error("Cel truncado");
          size_t w=u16(b,p+22), h=u16(b,p+24);
          if(!w || !h || w>2048 || h>2048) throw std::runtime_error("Cel demasiado grande");
          decoded += w*h*4;
          if(decoded>64u*1024u*1024u) throw std::runtime_error("Documento supera el limite de memoria alpha");
        } else if(kind!=1) throw std::runtime_error("Tipo de cel todavia no soportado");
      }
      p+=len;
    }
    pos=end;
  }
  if(pos!=b.size()) throw std::runtime_error("Datos finales no reconocidos");
}
void trim(std::vector<std::vector<uint8_t>>& stack) {
  size_t total=0; for(const auto& b:stack) total+=b.size();
  while(stack.size()>24 || (total>MAX_HISTORY && stack.size()>1)) { total-=stack.front().size(); stack.erase(stack.begin()); }
}
}
Editor::Editor(int w,int h) {
  if(w<1||h<1||w>512||h>512) throw std::runtime_error("Nuevo lienzo: de 1 a 512 pixeles por lado");
  sprite_.reset(doc::Sprite::MakeStdSprite(doc::ImageSpec(doc::ColorMode::RGB,w,h)));
}
Editor::~Editor() = default;
std::vector<int> Editor::info() const {
  return {sprite_->width(),sprite_->height(),sprite_->totalFrames(),frame_,int(editableLayers(sprite_.get()).size()),layer_,sprite_->frameDuration(frame_),int(undo_.size()),int(redo_.size())};
}
std::string Editor::layerNames() const {
  std::string out; for(auto* l:editableLayers(sprite_.get())) { if(!out.empty())out+='\n'; out+=(l->isVisible()?"[+] ":"[-] ")+l->name(); } return out;
}
std::vector<uint32_t> Editor::render() {
  std::unique_ptr<doc::Image> image(doc::Image::create(doc::IMAGE_RGB,sprite_->width(),sprite_->height()));
  image->clear(0); ::render::Render renderer; renderer.setNewBlend(true); renderer.renderSprite(image.get(),sprite_.get(),frame_);
  std::vector<uint32_t> out(size_t(image->width())*image->height());
  for(int y=0;y<image->height();++y)for(int x=0;x<image->width();++x) {
    auto c=image->getPixel(x,y); out[size_t(y)*image->width()+x]=(uint32_t(doc::rgba_geta(c))<<24)|(doc::rgba_getr(c)<<16)|(doc::rgba_getg(c)<<8)|doc::rgba_getb(c);
  } return out;
}
std::vector<uint8_t> Editor::save() {
  MemoryFile file(true); Writer delegate(sprite_.get()); dio::AsepriteEncoder encoder; encoder.initialize(&delegate,&file);
  if(!encoder.encode()||!file.ok()||!delegate.message.empty()) throw std::runtime_error("No se pudo guardar: "+delegate.message);
  return std::move(file.bytes);
}
void Editor::restore(const std::vector<uint8_t>& bytes) {
  preflight(bytes); MemoryFile file(bytes); Reader reader; dio::AsepriteDecoder decoder; decoder.initialize(&reader,&file);
  if(!decoder.decode()||!reader.result||!reader.message.empty()) throw std::runtime_error("No se pudo abrir: "+reader.message);
  auto layers=editableLayers(reader.result.get()); if(layers.empty()||layers.size()>64)throw std::runtime_error("Numero de capas no soportado");
  sprite_=std::move(reader.result); frame_=std::clamp(frame_,0,sprite_->totalFrames()-1); layer_=std::clamp(layer_,0,int(layers.size())-1);
}
void Editor::open(const std::vector<uint8_t>& bytes) { restore(bytes); undo_.clear();redo_.clear(); }
void Editor::checkpoint() { undo_.push_back(save()); trim(undo_);redo_.clear(); }
void Editor::beginStroke() {
  auto layers=editableLayers(sprite_.get()); auto* l=layers.at(layer_);
  if(!l->isVisibleHierarchy() || !l->isEditable()) throw std::runtime_error("La capa esta oculta o bloqueada");
  checkpoint(); auto* cel=l->cel(frame_);
  if(!cel) { doc::ImageRef image(doc::Image::create(doc::IMAGE_RGB,sprite_->width(),sprite_->height()));image->clear(0);cel=new doc::Cel(frame_,image);l->addCel(cel); }
  // Detach linked cel data and enlarge without discarding pixels outside the canvas.
  auto data=std::make_shared<doc::CelData>(*cel->data());
  int x0=std::min(0,cel->x()), y0=std::min(0,cel->y());
  int w=std::max(sprite_->width(),cel->x()+cel->image()->width())-x0;
  int h=std::max(sprite_->height(),cel->y()+cel->image()->height())-y0;
  if(w>2048||h>2048)throw std::runtime_error("Cel demasiado grande para editar");
  doc::ImageRef grown(doc::Image::create(doc::IMAGE_RGB,w,h));grown->clear(0);
  for(int y=0;y<cel->image()->height();++y)for(int x=0;x<cel->image()->width();++x)grown->putPixel(x+cel->x()-x0,y+cel->y()-y0,cel->image()->getPixel(x,y));
  data->setImage(grown,l);data->setPosition(gfx::Point(x0,y0));cel->setDataRef(data);
}
void Editor::line(int x0,int y0,int x1,int y1,uint32_t argb,int size,bool erase) {
  auto* l=editableLayers(sprite_.get()).at(layer_);auto* c=l->cel(frame_);if(!c)throw std::runtime_error("Trazo no iniciado");
  auto color=erase?0:doc::rgba((argb>>16)&255,(argb>>8)&255,argb&255,(argb>>24)&255);
  const int w=sprite_->width(),h=sprite_->height();size=std::clamp(size,1,32);
  x0=std::clamp(x0,-w,w*2);x1=std::clamp(x1,-w,w*2);y0=std::clamp(y0,-h,h*2);y1=std::clamp(y1,-h,h*2);
  int dx=std::abs(x1-x0),sx=x0<x1?1:-1,dy=-std::abs(y1-y0),sy=y0<y1?1:-1,err=dx+dy;
  for(;;) {
    for(int yy=y0-size/2;yy<y0-size/2+size;++yy)for(int xx=x0-size/2;xx<x0-size/2+size;++xx)if(xx>=0&&yy>=0&&xx<w&&yy<h)c->image()->putPixel(xx-c->x(),yy-c->y(),color);
    if(x0==x1&&y0==y1)break;int e=2*err;if(e>=dy){err+=dy;x0+=sx;}if(e<=dx){err+=dx;y0+=sy;}
  }
}
bool Editor::command(int op,int value) {
  auto layers=editableLayers(sprite_.get());
  if(op==0||op==1) {
    auto& from=op==0?undo_:redo_;auto& to=op==0?redo_:undo_;
    if(from.empty())return false;auto current=save();auto target=from.back();restore(target);from.pop_back();to.push_back(std::move(current));trim(to);return true;
  }
  if(op==4){frame_=std::clamp(value,0,sprite_->totalFrames()-1);return true;}
  if(op==5){layer_=std::clamp(value,0,int(layers.size())-1);return true;}
  if(op==2||op==3) {
    size_t n=size_t(sprite_->width())*sprite_->height()*(layers.size()+(op==3))*(sprite_->totalFrames()+(op==2));
    if(n>16u*1024u*1024u||sprite_->totalFrames()>=128||layers.size()>=64)throw std::runtime_error("Limite de memoria de la alpha alcanzado");
    checkpoint();
    if(op==2) { int dest=sprite_->totalFrames();sprite_->addFrame(dest);for(auto* l:layers)if(auto* c=l->cel(frame_)){auto* copy=doc::Cel::MakeCopy(dest,c);copy->data()->setUserData(c->data()->userData());l->addCel(copy);}sprite_->setFrameDuration(dest,sprite_->frameDuration(frame_));frame_=dest; }
    else {auto* l=new doc::LayerImage(sprite_.get());l->setName("Capa "+std::to_string(layers.size()+1));sprite_->root()->addLayer(l);layer_=int(layers.size());}
    return true;
  }
  if(op==6){checkpoint();layers.at(layer_)->setVisible(!layers.at(layer_)->isVisible());return true;}
  if(op==7){checkpoint();sprite_->setFrameDuration(frame_,std::clamp(value,20,5000));return true;}
  throw std::runtime_error("Operacion desconocida");
}
}
