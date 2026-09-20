#include "Controller.h"
#include "Editor.h"
#include "base/source/fstreamer.h"
#include <algorithm>
#include <cstring>
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
  if (id >= meterBase && id < meterBase + 5) meters_[id - meterBase].store(std::clamp(value, 0.0, 1.0), std::memory_order_relaxed);
  return EditControllerEx1::setParamNormalized(id, value);
}
tresult PLUGIN_API Controller::setComponentState(IBStream* stream) {
  if (!stream) return kInvalidArgument;
  IBStreamer reader(stream, kLittleEndian); int32 version = 0, bypass = 0;
  if (!reader.readInt32(version) || version != 1 || !reader.readInt32(bypass)) return kResultFalse;
  setParamNormalized(bypassId, bypass ? 1 : 0); return kResultOk;
}
}
