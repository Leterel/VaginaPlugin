#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <objbase.h>
#include "Ids.h"
#include "base/source/fobject.h"
#include "pluginterfaces/base/ipluginbase.h"
#include "pluginterfaces/vst/ivsteditcontroller.h"
#include "pluginterfaces/gui/iplugview.h"
#include "pluginterfaces/vst/ivstaudioprocessor.h"
#include "public.sdk/source/vst/hosting/hostclasses.h"
#include <cstdlib>
#include <iostream>
#include <limits>
#include "pluginterfaces/gui/iplugviewcontentscalesupport.h"

using namespace Steinberg;
using namespace Steinberg::Vst;
void require(bool ok, const char* what) {
  if (!ok) { std::cerr << "FAIL: " << what << '\n'; std::exit(1); }
}
void pump() {
  MSG message{};
  const auto until = GetTickCount64() + 90;
  do {
    while (PeekMessageW(&message, nullptr, 0, 0, PM_REMOVE)) {
      TranslateMessage(&message); DispatchMessageW(&message);
    }
    Sleep(2);
  } while (GetTickCount64() < until);
}
class TestHost final : public HostApplication {
public:
  int messages = 0;
  tresult PLUGIN_API createInstance(TUID cid, TUID iid, void** object) override {
    ++messages;
    return HostApplication::createInstance(cid, iid, object);
  }
};
class ResizeHost final : public FObject, public IPlugFrame {
public:
  bool reject = false;
  int requests = 0;
  tresult PLUGIN_API resizeView(IPlugView* view, ViewRect* size) override {
    ++requests;
    return reject ? kResultFalse : view->onSize(size);
  }
  DEFINE_INTERFACES
    DEF_INTERFACE(IPlugFrame)
  END_DEFINE_INTERFACES(FObject)
  REFCOUNT_METHODS(FObject)
};
void expectSize(IPlugView* view, int width, int height) {
  ViewRect size{};
  require(view->getSize(&size) == kResultOk && size.getWidth() == width && size.getHeight() == height,
          "host-visible scaled dimensions");
}
int wmain(int argc, wchar_t** argv) {
  require(argc == 2, "plugin DLL argument required");
  CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
  HMODULE library = LoadLibraryW(argv[1]);
  require(library != nullptr, "load compiled plugin");
  auto init = reinterpret_cast<bool (*)()>(GetProcAddress(library, "InitDll"));
  auto exitDll = reinterpret_cast<bool (*)()>(GetProcAddress(library, "ExitDll"));
  auto getFactory = reinterpret_cast<GetFactoryProc>(GetProcAddress(library, "GetPluginFactory"));
  require(init && exitDll && getFactory && init(), "initialize VST3 module");
  auto* factory = getFactory(); require(factory != nullptr, "plugin factory");
  auto host = owned(new TestHost);
  IEditController* controller = nullptr;
  require(factory->createInstance(Meme::controllerId, IEditController::iid,
          reinterpret_cast<void**>(&controller)) == kResultOk, "controller instance");
  require(controller->initialize(host) == kResultOk, "controller initialize");
  IComponent* component = nullptr;
  require(factory->createInstance(Meme::processorId, IComponent::iid,
          reinterpret_cast<void**>(&component)) == kResultOk, "processor instance");
  require(component->initialize(host) == kResultOk, "processor initialize");
  FUnknownPtr<IAudioProcessor> processor(component);
  FUnknownPtr<IConnectionPoint> processorConnection(component), controllerConnection(controller);
  require(processor && processorConnection && controllerConnection, "processor and connection interfaces");
  require(processorConnection->connect(controllerConnection) == kResultOk &&
          controllerConnection->connect(processorConnection) == kResultOk, "connect compiled components");
  ProcessSetup setup{}; setup.sampleRate = 48000; setup.maxSamplesPerBlock = 512; setup.symbolicSampleSize = kSample32;
  require(processor->setupProcessing(setup) == kResultOk && component->setActive(true) == kResultOk, "activate compiled processor");
  HWND window = CreateWindowExW(0, L"STATIC", L"VaginaPlugin test host",
                              WS_OVERLAPPEDWINDOW, 0, 0, 990, 650,
                              nullptr, nullptr, GetModuleHandleW(nullptr), nullptr);
  require(window != nullptr, "hidden native host window");
  for (int cycle = 0; cycle < 20; ++cycle) {
    require(processor->setProcessing(cycle % 2) == kResultOk, "change processing state");
    const int beforeOpen = host->messages;
    auto* view = controller->createView(ViewType::kEditor);
    require(view != nullptr, "create editor view");
    require(view->isPlatformTypeSupported(kPlatformTypeHWND) == kResultTrue, "HWND support");
    require(view->attached(window, kPlatformTypeHWND) == kResultOk, "attach editor");
    ViewRect size;
    require(view->getSize(&size) == kResultOk && size.getWidth() == 960 && size.getHeight() == 600,
            "editor dimensions");
    for (int band = 0; band < 5; ++band)
      require(controller->setParamNormalized(Meme::meterBase + band, cycle % 2 ? 0.9 : 0.0) == kResultOk,
              "deliver live meter updates");
    pump();
    require(host->messages >= beforeOpen + 4, "editor timer exchanges real processor snapshots");
    require(view->removed() == kResultOk, "remove editor");
    view->release();
    const int afterClose = host->messages;
    pump();
    require(host->messages == afterClose, "closed editor stops polling");
  }
  // Some DAWs keep one IPlugView alive while closing and reopening its window.
  auto* reusedView = controller->createView(ViewType::kEditor);
  require(reusedView != nullptr, "create reused editor view");
  for (int cycle = 0; cycle < 20; ++cycle) {
    const int beforeOpen = host->messages;
    require(reusedView->attached(window, kPlatformTypeHWND) == kResultOk, "reattach same editor view");
    pump();
    require(host->messages >= beforeOpen + 4, "reopened editor restarts snapshot polling");
    require(reusedView->removed() == kResultOk, "remove reused editor view");
  }
  reusedView->release();
  // Exercise the optional VST3 scaling interface against the compiled DLL.
  auto* scaledView = controller->createView(ViewType::kEditor);
  FUnknownPtr<IPlugViewContentScaleSupport> scaling(scaledView);
  require(scaling != nullptr, "content scale interface available");
  require(scaling->setContentScaleFactor(1.5f) == kResultTrue, "scale before frame/attachment");
  expectSize(scaledView, 1440, 900);
  auto resizeHost = owned(new ResizeHost);
  require(scaledView->setFrame(resizeHost) == kResultTrue, "install resize host");
  for (int cycle = 0; cycle < 4; ++cycle) {
    require(scaledView->attached(window, kPlatformTypeHWND) == kResultOk, "attach scaled editor");
    for (float factor : {1.0f, 1.25f, 1.5f, 2.0f, 3.0f, 4.0f, 0.5f, 1.5f}) {
      require(scaling->setContentScaleFactor(factor) == kResultTrue, "change scale while open");
      expectSize(scaledView, int(960 * factor), int(600 * factor));
      HWND child = GetWindow(window, GW_CHILD);
      RECT client{};
      require(child && GetClientRect(child, &client) && client.right == int(960 * factor) &&
              client.bottom == int(600 * factor), "native child follows content scale");
      pump();
    }
    const int beforeInvalid = resizeHost->requests;
    for (float invalid : {0.f, -1.f, 4.1f, std::numeric_limits<float>::infinity(),
                          std::numeric_limits<float>::quiet_NaN()})
      require(scaling->setContentScaleFactor(invalid) == kInvalidArgument, "reject invalid scale");
    require(resizeHost->requests == beforeInvalid, "invalid scale never resizes host");
    resizeHost->reject = true;
    require(scaling->setContentScaleFactor(2.f) == kResultFalse, "respect host resize refusal");
    expectSize(scaledView, 1440, 900);
    resizeHost->reject = false;
    require(scaledView->removed() == kResultOk, "remove scaled editor");
    const int afterClose = host->messages;
    pump();
    require(host->messages == afterClose, "scaled closed editor stops polling");
  }
  scaledView->setFrame(nullptr);
  scaling = nullptr;
  scaledView->release();
  DestroyWindow(window);
  require(processor->setProcessing(false) == kResultOk && component->setActive(false) == kResultOk, "deactivate processor");
  controllerConnection->disconnect(processorConnection); processorConnection->disconnect(controllerConnection);
  controllerConnection = nullptr; processorConnection = nullptr; processor = nullptr;
  require(component->terminate() == kResultOk, "terminate processor"); component->release();
  require(controller->terminate() == kResultOk, "terminate controller");
  controller->release(); factory->release();
  require(exitDll(), "terminate VST3 module"); FreeLibrary(library); CoUninitialize();
  std::cout << "PASS: compiled VST3; 20 new-view and 20 reused-view cycles; real snapshot timer starts/stops; 32 live scale changes and reopen/refusal checks; clean unload\n";
}
