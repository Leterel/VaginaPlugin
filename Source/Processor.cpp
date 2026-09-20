#include "Processor.h"
#include "pluginterfaces/vst/ivstparameterchanges.h"
#include "base/source/fstreamer.h"
#include <cmath>
using namespace Steinberg;
using namespace Steinberg::Vst;
namespace Meme {
Processor::Processor() { setControllerClass(controllerId); }
tresult PLUGIN_API Processor::initialize(FUnknown* host) {
  auto result = AudioEffect::initialize(host);
  if (result != kResultOk) return result;
  addAudioInput(STR16("Audio in"), SpeakerArr::kStereo);
  addAudioOutput(STR16("Unchanged audio"), SpeakerArr::kStereo);
  return kResultOk;
}
tresult PLUGIN_API Processor::setupProcessing(ProcessSetup& setup) {
  if (!std::isfinite(setup.sampleRate) || setup.sampleRate <= 0) return kInvalidArgument;
  analyzer_.prepare(setup.sampleRate);
  return AudioEffect::setupProcessing(setup);
}
tresult PLUGIN_API Processor::setActive(TBool active) { if (active) analyzer_.reset(); return AudioEffect::setActive(active); }
tresult PLUGIN_API Processor::canProcessSampleSize(int32 format) { return format == kSample32 || format == kSample64 ? kResultTrue : kResultFalse; }
tresult PLUGIN_API Processor::setBusArrangements(SpeakerArrangement* input, int32 ni, SpeakerArrangement* output, int32 no) {
  if (ni != 1 || no != 1 || !input || !output || input[0] != output[0] || (input[0] != SpeakerArr::kMono && input[0] != SpeakerArr::kStereo)) return kResultFalse;
  return AudioEffect::setBusArrangements(input, ni, output, no);
}
tresult PLUGIN_API Processor::process(ProcessData& data) {
  if (data.inputParameterChanges) {
    for (int32 p = 0; p < data.inputParameterChanges->getParameterCount(); ++p) {
      auto* queue = data.inputParameterChanges->getParameterData(p);
      if (queue && queue->getParameterId() == bypassId && queue->getPointCount() > 0) {
        int32 offset; ParamValue value;
        if (queue->getPoint(queue->getPointCount() - 1, offset, value) == kResultOk) bypass_ = value > 0.5;
      }
    }
  }
  if (data.numSamples <= 0 || data.numInputs < 1 || data.numOutputs < 1 || !data.inputs || !data.outputs) return kResultOk;
  auto& in = data.inputs[0]; auto& out = data.outputs[0];
  if (in.numChannels != out.numChannels || in.numChannels < 1 || in.numChannels > 2) return kInvalidArgument;
  out.silenceFlags = in.silenceFlags;
  const auto previousFrame = analyzer_.frames();
  if (data.symbolicSampleSize == kSample32) {
    if (!in.channelBuffers32 || !out.channelBuffers32) return kInvalidArgument;
    analyzer_.feed(in.channelBuffers32, in.numChannels, data.numSamples, in.silenceFlags);
    copyAudio(in.channelBuffers32, out.channelBuffers32, in.numChannels, data.numSamples, in.silenceFlags);
  } else if (data.symbolicSampleSize == kSample64) {
    if (!in.channelBuffers64 || !out.channelBuffers64) return kInvalidArgument;
    analyzer_.feed(in.channelBuffers64, in.numChannels, data.numSamples, in.silenceFlags);
    copyAudio(in.channelBuffers64, out.channelBuffers64, in.numChannels, data.numSamples, in.silenceFlags);
  } else return kInvalidArgument;
  if (previousFrame != analyzer_.frames() && data.outputParameterChanges) {
    for (int band = 0; band < 5; ++band) {
      int32 queueIndex = 0, pointIndex = 0;
      auto* queue = data.outputParameterChanges->addParameterData(meterBase + band, queueIndex);
      const double rms = analyzer_.levels()[band];
      const double meter = bypass_ ? 0 : std::clamp((20 * std::log10(std::max(rms, 1e-12)) + 60) / 60, 0.0, 1.0);
      if (queue) queue->addPoint(data.numSamples - 1, meter, pointIndex);
    }
  }
  return kResultOk;
}
tresult PLUGIN_API Processor::setState(IBStream* stream) {
  if (!stream) return kInvalidArgument;
  IBStreamer reader(stream, kLittleEndian); int32 version = 0, bypass = 0;
  if (!reader.readInt32(version) || version != 1 || !reader.readInt32(bypass)) return kResultFalse;
  bypass_ = bypass != 0; return kResultOk;
}
tresult PLUGIN_API Processor::getState(IBStream* stream) {
  if (!stream) return kInvalidArgument;
  IBStreamer writer(stream, kLittleEndian);
  return writer.writeInt32(1) && writer.writeInt32(bypass_ ? 1 : 0) ? kResultOk : kResultFalse;
}
}
