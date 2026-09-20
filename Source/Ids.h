#pragma once
#include "pluginterfaces/base/funknown.h"
#include "pluginterfaces/vst/vsttypes.h"
namespace Meme {
inline const Steinberg::FUID processorId(0x8A9DC5A1,0xD7EF4AE5,0xA134380C,0x1E02D811);
inline const Steinberg::FUID controllerId(0x7C35FD93,0x0DE942D4,0xBA431EA8,0x3DF1511B);
inline constexpr Steinberg::Vst::ParamID meterBase = 100;
inline constexpr Steinberg::Vst::ParamID bypassId = 10;
}
