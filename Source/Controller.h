#pragma once
#include "Ids.h"
#include "public.sdk/source/vst/vsteditcontroller.h"
#include <array>
#include <atomic>
#include <chrono>
namespace Meme {
class Controller final : public Steinberg::Vst::EditControllerEx1 {
public:
  static Steinberg::FUnknown* create(void*) { return static_cast<Steinberg::Vst::IEditController*>(new Controller); }
  Steinberg::tresult PLUGIN_API initialize(Steinberg::FUnknown*) override;
  Steinberg::IPlugView* PLUGIN_API createView(Steinberg::FIDString) override;
  Steinberg::tresult PLUGIN_API setParamNormalized(Steinberg::Vst::ParamID, Steinberg::Vst::ParamValue) override;
  Steinberg::tresult PLUGIN_API setComponentState(Steinberg::IBStream*) override;
  Steinberg::tresult PLUGIN_API notify(Steinberg::Vst::IMessage*) override;
  Steinberg::tresult PLUGIN_API disconnect(Steinberg::Vst::IConnectionPoint*) override;
  void requestMeterSnapshot(); // UI thread, called only while an editor is open.
  double meter(int band) const noexcept;
private:
  std::array<std::atomic<double>, 5> meters_{};
  std::array<double, 5> snapshotMeters_{};
  bool haveSnapshot_ = false, processing_ = false;
  Steinberg::int64 sequence_ = 0;
  double idleGraceSeconds_ = 0.75;
  std::chrono::steady_clock::time_point lastAdvance_{};
};
}
