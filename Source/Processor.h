#pragma once
#include "Analyzer.h"
#include "Ids.h"
#include "public.sdk/source/vst/vstaudioeffect.h"
namespace Meme {
class Processor final : public Steinberg::Vst::AudioEffect {
public:
  Processor();
  static Steinberg::FUnknown* create(void*) { return static_cast<Steinberg::Vst::IAudioProcessor*>(new Processor); }
  Steinberg::tresult PLUGIN_API initialize(Steinberg::FUnknown*) override;
  Steinberg::tresult PLUGIN_API setupProcessing(Steinberg::Vst::ProcessSetup&) override;
  Steinberg::tresult PLUGIN_API setActive(Steinberg::TBool) override;
  Steinberg::tresult PLUGIN_API setBusArrangements(Steinberg::Vst::SpeakerArrangement*, Steinberg::int32, Steinberg::Vst::SpeakerArrangement*, Steinberg::int32) override;
  Steinberg::tresult PLUGIN_API canProcessSampleSize(Steinberg::int32) override;
  Steinberg::tresult PLUGIN_API process(Steinberg::Vst::ProcessData&) override;
  Steinberg::tresult PLUGIN_API setState(Steinberg::IBStream*) override;
  Steinberg::tresult PLUGIN_API getState(Steinberg::IBStream*) override;
  const std::array<double, 5>& bandLevels() const noexcept { return analyzer_.levels(); }
private:
  Analyzer analyzer_;
  bool bypass_ = false;
};
}
