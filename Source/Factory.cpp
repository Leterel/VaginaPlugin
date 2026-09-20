#include "Processor.h"
#include "Controller.h"
#include "public.sdk/source/main/pluginfactory.h"
using namespace Steinberg;
using namespace Steinberg::Vst;
BEGIN_FACTORY_DEF("Leterel", "https://github.com/Leterel/VaginaPlugin", "")
DEF_CLASS2(INLINE_UID_FROM_FUID(Meme::processorId), PClassInfo::kManyInstances, kVstAudioEffectClass, "VaginaPlugin", Vst::kDistributable, "Fx|Analyzer", "0.1.0", kVstVersionString, Meme::Processor::create)
DEF_CLASS2(INLINE_UID_FROM_FUID(Meme::controllerId), PClassInfo::kManyInstances, kVstComponentControllerClass, "VaginaPlugin Controller", 0, "", "0.1.0", kVstVersionString, Meme::Controller::create)
END_FACTORY
