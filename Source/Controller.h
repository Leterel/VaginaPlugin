#pragma once
#include "Ids.h"
#include "public.sdk/source/vst/vsteditcontroller.h"
#include <array>
#include <atomic>
namespace Meme {
class Controller final : public Steinberg::Vst::EditControllerEx1 {
public:
  static Steinberg::FUnknown* create(void*) { return static_cast<Steinberg::Vst::IEditController*>(new Controller); }
  Steinberg::tresult PLUGIN_API initialize(Steinberg::FUnknown*) override;
  Steinberg::IPlugView* PLUGIN_API createView(Steinberg::FIDString) override;
  Steinberg::tresult PLUGIN_API setParamNormalized(Steinberg::Vst::ParamID, Steinberg::Vst::ParamValue) override;
  Steinberg::tresult PLUGIN_API setComponentState(Steinberg::IBStream*) override;
  double meter(int band) const noexcept { return meters_[band].load(std::memory_order_relaxed); }
private:
  std::array<std::atomic<double>, 5> meters_{};
};
}
