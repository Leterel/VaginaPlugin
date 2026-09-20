#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <objbase.h>
#include "Ids.h"
#include "pluginterfaces/base/ipluginbase.h"
#include "pluginterfaces/vst/ivsteditcontroller.h"
#include "pluginterfaces/gui/iplugview.h"
#include <cstdlib>
#include <iostream>

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
  IEditController* controller = nullptr;
  require(factory->createInstance(Meme::controllerId, IEditController::iid,
          reinterpret_cast<void**>(&controller)) == kResultOk, "controller instance");
  require(controller->initialize(nullptr) == kResultOk, "controller initialize");
  HWND window = CreateWindowExW(0, L"STATIC", L"VaginaPlugin test host",
                              WS_OVERLAPPEDWINDOW, 0, 0, 990, 650,
                              nullptr, nullptr, GetModuleHandleW(nullptr), nullptr);
  require(window != nullptr, "hidden native host window");
  for (int cycle = 0; cycle < 20; ++cycle) {
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
    require(view->removed() == kResultOk, "remove editor");
    view->release();
  }
  DestroyWindow(window);
  require(controller->terminate() == kResultOk, "terminate controller");
  controller->release(); factory->release();
  require(exitDll(), "terminate VST3 module"); FreeLibrary(library); CoUninitialize();
  std::cout << "PASS: compiled VST3 loaded; 20 hidden native editor attach/update/detach cycles; clean unload\n";
}
