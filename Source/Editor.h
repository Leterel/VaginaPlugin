#pragma once
#include "public.sdk/source/vst/vstguieditor.h"
#include "pluginterfaces/gui/iplugviewcontentscalesupport.h"
namespace Meme {
class Controller;
class VisualView;
VSTGUI::CView* createVisualView(Controller*);
class Editor final : public Steinberg::Vst::VSTGUIEditor, public Steinberg::IPlugViewContentScaleSupport {
public:
  explicit Editor(Controller*);
  ~Editor() override;
  bool PLUGIN_API open(void*, const VSTGUI::PlatformType&) override;
  void PLUGIN_API close() override;
  Steinberg::tresult PLUGIN_API setContentScaleFactor(ScaleFactor factor) override;
  OBJ_METHODS(Editor, Steinberg::Vst::VSTGUIEditor)
  DEFINE_INTERFACES
    DEF_INTERFACE(Steinberg::IPlugViewContentScaleSupport)
  END_DEFINE_INTERFACES(Steinberg::Vst::VSTGUIEditor)
  REFCOUNT_METHODS(Steinberg::Vst::VSTGUIEditor)
private:
  Controller* owner_;
  double contentScale_ = 1.0;
};
}
