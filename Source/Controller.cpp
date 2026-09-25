#include "Controller.h"
#include "Editor.h"
#include "MeterMessages.h"
#include "base/source/fstreamer.h"
#include <algorithm>
#include <cstring>
#include <cmath>
using namespace Steinberg;
using namespace Steinberg::Vst;
namespace Meme {
tresult PLUGIN_API Controller::initialize(FUnknown* host) {
  auto result = EditControllerEx1::initialize(host); if (result != kResultOk) return result;
  const TChar* names[] = { STR16("Cylinder / sub"), STR16("Flow / bass"), STR16("Arc / mid"), STR16("Drops / high"), STR16("Particles / air") };
  for (int band = 0; band < 5; ++band) parameters.addParameter(names[band], STR16(""), 0, 0, ParameterInfo::kIsReadOnly, meterBase + band);
  parameters.addParameter(STR16("Bypass visualizer"), nullptr, 1, 0, ParameterInfo::kCanAutomate | ParameterInfo::kIsBypass, bypassId);
  return kResultOk;
}
IPlugView* PLUGIN_API Controller::createView(FIDString name) { return name && std::strcmp(name, ViewType::kEditor) == 0 ? new Editor(this) : nullptr; }
tresult PLUGIN_API Controller::setParamNormalized(ParamID id, ParamValue value) {
  if (id >= meterBase && id < meterBase + 5) {
    value = std::isfinite(value) ? std::clamp(value, 0.0, 1.0) : 0.0;
    meters_[id - meterBase].store(value, std::memory_order_relaxed);
  }
  return EditControllerEx1::setParamNormalized(id, value);
}

void Controller::requestMeterSnapshot() {
  if (!getPeer()) return;
  if (auto message = owned(allocateMessage())) {
    message->setMessageID(meterRequest);
    sendMessage(message);
  }
}

tresult PLUGIN_API Controller::notify(IMessage* message) {
  if (!message || !message->getMessageID()) return kInvalidArgument;
  if (std::strcmp(message->getMessageID(), meterSnapshot) != 0) return EditControllerEx1::notify(message);
  auto* attributes = message->getAttributes();
  int64 processing = 0, sequence = 0;
  double grace = 0;
  std::array<double, 5> values{};
  if (!attributes || attributes->getInt("processing", processing) != kResultOk ||
      (processing != 0 && processing != 1) ||
      attributes->getInt("sequence", sequence) != kResultOk ||
      attributes->getFloat("idleGraceSeconds", grace) != kResultOk ||
      !std::isfinite(grace) || grace < 0.75) return kInvalidArgument;
  for (int band = 0; band < 5; ++band) {
    if (attributes->getFloat(meterKeys[band], values[band]) != kResultOk || !std::isfinite(values[band])) return kInvalidArgument;
    values[band] = std::clamp(values[band], 0.0, 1.0);
  }
  if (!haveSnapshot_ || sequence != sequence_ || (!processing_ && processing))
    lastAdvance_ = std::chrono::steady_clock::now();
  snapshotMeters_ = values;
  sequence_ = sequence;
  processing_ = processing != 0;
  idleGraceSeconds_ = grace;
  haveSnapshot_ = true;
  return kResultOk;
}

double Controller::meter(int band) const noexcept {
  if (!haveSnapshot_) return meters_[band].load(std::memory_order_relaxed);
  // Host parameter queues can arrive late or deduplicate steady values. Once
  // available, actual processor snapshots are authoritative for the visualizer.
  if (!processing_ || std::chrono::duration<double>(std::chrono::steady_clock::now() - lastAdvance_).count() >= idleGraceSeconds_) return 0;
  return snapshotMeters_[band];
}

tresult PLUGIN_API Controller::disconnect(IConnectionPoint* other) {
  auto result = EditControllerEx1::disconnect(other);
  if (result == kResultOk) {
    haveSnapshot_ = false;
    for (auto& meter : meters_) meter.store(0, std::memory_order_relaxed);
  }
  return result;
}
tresult PLUGIN_API Controller::setComponentState(IBStream* stream) {
  if (!stream) return kInvalidArgument;
  IBStreamer reader(stream, kLittleEndian); int32 version = 0, bypass = 0;
  if (!reader.readInt32(version) || version != 1 || !reader.readInt32(bypass)) return kResultFalse;
  setParamNormalized(bypassId, bypass ? 1 : 0); return kResultOk;
}
}
