#pragma once
#include "public.sdk/source/vst/vstguieditor.h"
namespace Meme {
class Controller;
class VisualView;
VSTGUI::CView* createVisualView(Controller*);
class Editor final : public Steinberg::Vst::VSTGUIEditor {
public:
  explicit Editor(Controller*);
  ~Editor() override;
  bool PLUGIN_API open(void*, const VSTGUI::PlatformType&) override;
  void PLUGIN_API close() override;
private:
  Controller* owner_;
};
}
