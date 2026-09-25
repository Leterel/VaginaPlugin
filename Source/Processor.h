#pragma once
#include "Analyzer.h"
#include "Ids.h"
#include "public.sdk/source/vst/vstaudioeffect.h"
#include <atomic>
namespace Meme {
class Processor final : public Steinberg::Vst::AudioEffect {
public:
  Processor();
  static Steinberg::FUnknown* create(void*) { return static_cast<Steinberg::Vst::IAudioProcessor*>(new Processor); }
  Steinberg::tresult PLUGIN_API initialize(Steinberg::FUnknown*) override;
  Steinberg::tresult PLUGIN_API setupProcessing(Steinberg::Vst::ProcessSetup&) override;
  Steinberg::tresult PLUGIN_API setActive(Steinberg::TBool) override;
  Steinberg::tresult PLUGIN_API setProcessing(Steinberg::TBool) override;
  Steinberg::tresult PLUGIN_API notify(Steinberg::Vst::IMessage*) override;
  Steinberg::tresult PLUGIN_API setBusArrangements(Steinberg::Vst::SpeakerArrangement*, Steinberg::int32, Steinberg::Vst::SpeakerArrangement*, Steinberg::int32) override;
  Steinberg::tresult PLUGIN_API canProcessSampleSize(Steinberg::int32) override;
  Steinberg::tresult PLUGIN_API process(Steinberg::Vst::ProcessData&) override;
  Steinberg::tresult PLUGIN_API setState(Steinberg::IBStream*) override;
  Steinberg::tresult PLUGIN_API getState(Steinberg::IBStream*) override;
  const std::array<double, 5>& bandLevels() const noexcept { return analyzer_.levels(); }
private:
  Analyzer analyzer_;
  bool bypass_ = false;
  // Written by audio, read only by the UI-thread message handler. No timers or
  // host messaging run in process() or setProcessing().
  static_assert(std::atomic<double>::is_always_lock_free);
  static_assert(std::atomic<Steinberg::int64>::is_always_lock_free);
  static_assert(std::atomic<bool>::is_always_lock_free);
  std::array<std::atomic<double>, 5> uiMeters_{};
  std::atomic<bool> processing_{false};
  std::atomic<Steinberg::int64> processSequence_{0};
  double idleGraceSeconds_ = 0.75; // Set during setup, read on the UI thread.
};
}
