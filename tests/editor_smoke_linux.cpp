#include "Ids.h"
#include "pluginterfaces/base/funknownimpl.h"
#include "pluginterfaces/gui/iplugview.h"
#include "pluginterfaces/vst/ivstaudioprocessor.h"
#include "pluginterfaces/vst/ivsteditcontroller.h"
#include "public.sdk/source/vst/hosting/hostclasses.h"
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <dlfcn.h>
#include <poll.h>
#include <algorithm>
#include <chrono>
#include <cstdlib>
#include <iostream>
#include <fstream>
#include <map>
#include <set>
#include <vector>

using namespace Steinberg;
using namespace Steinberg::Vst;
using Clock = std::chrono::steady_clock;
static void require(bool ok, const char* description) {
  if (!ok) { std::cerr << "FAIL: " << description << '\n'; std::exit(1); }
}

// A real VST3 Linux host must provide the event loop. Run it on this one UI
// thread, retaining handlers while callbacks can unregister themselves.
class Frame final : public U::Implements<U::Directly<IPlugFrame, Linux::IRunLoop>> {
public:
  tresult PLUGIN_API resizeView(IPlugView* view, ViewRect* size) override {
    return view && size ? view->onSize(size) : kInvalidArgument;
  }
  tresult PLUGIN_API registerEventHandler(Linux::IEventHandler* handler, int fd) override {
    if (!handler || fd < 0 || events.count(fd)) return kInvalidArgument;
    events.emplace(fd, handler); return kResultOk;
  }
  tresult PLUGIN_API unregisterEventHandler(Linux::IEventHandler* handler) override {
    auto found = std::find_if(events.begin(), events.end(), [handler](const auto& entry) {
      return entry.second.get() == handler;
    });
    if (found == events.end()) return kResultFalse;
    events.erase(found); return kResultOk;
  }
  tresult PLUGIN_API registerTimer(Linux::ITimerHandler* handler, Linux::TimerInterval interval) override {
    if (!handler || !interval || timers.count(handler)) return kInvalidArgument;
    auto duration = std::chrono::milliseconds(interval);
    timers.emplace(handler, Timer{handler, duration, Clock::now() + duration});
    return kResultOk;
  }
  tresult PLUGIN_API unregisterTimer(Linux::ITimerHandler* handler) override {
    return timers.erase(handler) ? kResultOk : kResultFalse;
  }
  void pump(Display* display) {
    const auto until = Clock::now() + std::chrono::milliseconds(180);
    do {
      while (XPending(display)) { XEvent event{}; XNextEvent(display, &event); }
      std::vector<pollfd> descriptors;
      for (const auto& entry : events) descriptors.push_back({entry.first, POLLIN, 0});
      if (poll(descriptors.data(), descriptors.size(), 2) > 0) {
        for (const auto& descriptor : descriptors) {
          auto found = events.find(descriptor.fd);
          if ((descriptor.revents & POLLIN) && found != events.end()) {
            auto handler = found->second; handler->onFDIsSet(descriptor.fd);
          }
        }
      }
      std::vector<IPtr<Linux::ITimerHandler>> due;
      const auto now = Clock::now();
      for (auto& entry : timers) {
        if (entry.second.next <= now) {
          entry.second.next = now + entry.second.interval;
          due.push_back(entry.second.handler);
        }
      }
      for (const auto& handler : due) if (timers.count(handler)) handler->onTimer();
    } while (Clock::now() < until);
  }
  bool empty() const { return events.empty() && timers.empty(); }
private:
  struct Timer {
    IPtr<Linux::ITimerHandler> handler;
    std::chrono::milliseconds interval;
    Clock::time_point next;
  };
  std::map<int, IPtr<Linux::IEventHandler>> events;
  std::map<Linux::ITimerHandler*, Timer> timers;
};

class TestHost final : public HostApplication {
public:
  explicit TestHost(Frame* frame) : frame(frame) {}
  tresult PLUGIN_API queryInterface(const TUID iid, void** result) override {
    if (FUnknownPrivate::iidEqual(iid, Linux::IRunLoop::iid)) return frame->queryInterface(iid, result);
    return HostApplication::queryInterface(iid, result);
  }
  tresult PLUGIN_API createInstance(TUID cid, TUID iid, void** object) override {
    ++messages; return HostApplication::createInstance(cid, iid, object);
  }
  int messages = 0;
private:
  IPtr<Frame> frame;
};

int main(int argc, char** argv) {
  require(argc == 2, "compiled Linux plugin .so argument required");
  auto* display = XOpenDisplay(nullptr);
  require(display != nullptr, "X display (run under xvfb-run for headless verification)");
  auto library = dlopen(argv[1], RTLD_NOW | RTLD_LOCAL);
  if (!library) std::cerr << dlerror() << '\n';
  require(library != nullptr, "load compiled Linux VST3 shared object");
  auto init = reinterpret_cast<bool (*)(void*)>(dlsym(library, "ModuleEntry"));
  auto finish = reinterpret_cast<bool (*)()>(dlsym(library, "ModuleExit"));
  auto getFactory = reinterpret_cast<GetFactoryProc>(dlsym(library, "GetPluginFactory"));
  require(init && finish && getFactory && init(library), "initialize Linux VST3 module");
  auto* factory = getFactory(); require(factory != nullptr, "plugin factory");
  auto frame = owned(new Frame);
  auto host = owned(new TestHost(frame));
  FUnknownPtr<IPluginFactory3> factory3(factory);
  require(factory3 && factory3->setHostContext(host) == kResultOk, "provide host run loop");
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
  ProcessSetup setup{}; setup.sampleRate = 48000; setup.maxSamplesPerBlock = 512;
  setup.symbolicSampleSize = kSample32;
  require(processor->setupProcessing(setup) == kResultOk && component->setActive(true) == kResultOk,
          "activate compiled processor");
  const auto window = XCreateSimpleWindow(display, DefaultRootWindow(display), 0, 0, 960, 600, 0, 0, 0);
  XMapWindow(display, window); XSync(display, False);
  auto attach = [&](IPlugView* view) {
    require(view && view->setFrame(frame) == kResultOk, "set host frame");
    require(view->isPlatformTypeSupported(kPlatformTypeX11EmbedWindowID) == kResultTrue, "X11 embedding");
    require(view->attached(reinterpret_cast<void*>(window), kPlatformTypeX11EmbedWindowID) == kResultOk,
            "attach Linux editor");
    ViewRect size{};
    require(view->getSize(&size) == kResultOk && size.getWidth() == 960 && size.getHeight() == 600,
            "editor dimensions");
    const auto before = host->messages;
    frame->pump(display);
    // Complete the XEmbed host handshake so the plugin maps its child window.
    Window root{}, parent{}, *children = nullptr; unsigned int count = 0;
    require(XQueryTree(display, window, &root, &parent, &children, &count) && count > 0, "embedded child window");
    for (unsigned int i = 0; i < count; ++i) {
      XEvent event{}; event.xclient.type = ClientMessage; event.xclient.window = children[i];
      event.xclient.message_type = XInternAtom(display, "_XEMBED", False); event.xclient.format = 32;
      event.xclient.data.l[0] = CurrentTime; event.xclient.data.l[1] = 0; // XEMBED_EMBEDDED_NOTIFY
      event.xclient.data.l[3] = window; event.xclient.data.l[4] = 0;
      XSendEvent(display, children[i], False, NoEventMask, &event);
    }
    XFree(children); XFlush(display); frame->pump(display);
    require(host->messages >= before + 4, "editor timer exchanges processor snapshots");
  };
  auto detach = [&](IPlugView* view) {
    require(view->removed() == kResultOk, "remove editor");
    const auto after = host->messages;
    frame->pump(display);
    require(host->messages == after, "closed editor stops polling");
  };
  for (int cycle = 0; cycle < 20; ++cycle) {
    require(processor->setProcessing(cycle % 2) == kResultOk, "change processing state");
    auto* view = controller->createView(ViewType::kEditor);
    attach(view);
    if (cycle == 0) {
      auto* pixels = XGetImage(display, window, 0, 0, 960, 600, AllPlanes, ZPixmap);
      require(pixels != nullptr, "capture only the synthetic host window");
      std::set<unsigned long> colors;
      for (int y = 0; y < 600; y += 3) for (int x = 0; x < 960; x += 3) colors.insert(XGetPixel(pixels, x, y));
      std::ofstream capture("editor-linux.ppm", std::ios::binary);
      capture << "P6\n960 600\n255\n";
      for (int y = 0; y < 600; ++y) for (int x = 0; x < 960; ++x) {
        auto pixel = XGetPixel(pixels, x, y);
        const char rgb[] = {static_cast<char>((pixel >> 16) & 255), static_cast<char>((pixel >> 8) & 255), static_cast<char>(pixel & 255)};
        capture.write(rgb, 3);
      }
      std::cout << "Captured synthetic host window: " << colors.size() << " sampled colors\n";
      XDestroyImage(pixels);
      require(colors.size() > 30, "editor draws actual colored graphics and text");
    }
    detach(view); view->setFrame(nullptr); view->release();
  }
  auto* reused = controller->createView(ViewType::kEditor);
  for (int cycle = 0; cycle < 20; ++cycle) { attach(reused); detach(reused); }
  reused->setFrame(nullptr); reused->release();
  XDestroyWindow(display, window); XCloseDisplay(display);
  require(processor->setProcessing(false) == kResultOk && component->setActive(false) == kResultOk,
          "deactivate processor");
  controllerConnection->disconnect(processorConnection); processorConnection->disconnect(controllerConnection);
  controllerConnection = nullptr; processorConnection = nullptr; processor = nullptr;
  require(component->terminate() == kResultOk, "terminate processor"); component->release();
  require(controller->terminate() == kResultOk, "terminate controller"); controller->release();
  factory3->setHostContext(nullptr); factory3 = nullptr; factory->release();
  require(finish(), "terminate Linux module");
  require(frame->empty(), "module unregisters every timer and file descriptor");
  dlclose(library);
  std::cout << "PASS: Linux compiled VST3; real X11 graphics; 20 new-view and 20 reused-view cycles; "
               "snapshot timer starts/stops; clean unload\n";
}
