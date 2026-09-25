#include "Controller.h"
#include "MeterMessages.h"
#include "Processor.h"
#include "pluginterfaces/vst/ivstprocesscontext.h"
#include "public.sdk/source/vst/hosting/hostclasses.h"
#include <array>
#include <chrono>
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <limits>
#include <new>
#include <thread>
// This console test links the editor but never opens it. The SDK requires the
// module symbol normally supplied by its DLL entry point.
void* moduleHandle = nullptr;
#if defined(_WIN32)
#include <malloc.h>
#endif

// Count C++ allocations only inside the production realtime callbacks below.
// Fixtures, assertions, host messages and sleeping all run outside these scopes.
namespace RealtimeAudit {
thread_local bool inside = false;
thread_local std::size_t allocations = 0;
void* allocate(std::size_t size) {
  if (inside) ++allocations;
  if (auto* result = std::malloc(size ? size : 1)) return result;
  throw std::bad_alloc();
}
void* allocateAligned(std::size_t size, std::size_t alignment) {
  if (inside) ++allocations;
#if defined(_WIN32)
  if (auto* result = _aligned_malloc(size ? size : 1, alignment)) return result;
#else
  void* result = nullptr;
  if (posix_memalign(&result, alignment, size ? size : 1) == 0) return result;
#endif
  throw std::bad_alloc();
}
void freeAligned(void* value) noexcept {
#if defined(_WIN32)
  _aligned_free(value);
#else
  std::free(value);
#endif
}
struct Scope {
  Scope() { inside = true; }
  ~Scope() { inside = false; }
};
}
void* operator new(std::size_t size) { return RealtimeAudit::allocate(size); }
void* operator new[](std::size_t size) { return RealtimeAudit::allocate(size); }
void operator delete(void* value) noexcept { std::free(value); }
void operator delete[](void* value) noexcept { std::free(value); }
void operator delete(void* value, std::size_t) noexcept { std::free(value); }
void operator delete[](void* value, std::size_t) noexcept { std::free(value); }
void* operator new(std::size_t size, std::align_val_t alignment) {
  return RealtimeAudit::allocateAligned(size, static_cast<std::size_t>(alignment));
}
void* operator new[](std::size_t size, std::align_val_t alignment) {
  return RealtimeAudit::allocateAligned(size, static_cast<std::size_t>(alignment));
}
void operator delete(void* value, std::align_val_t) noexcept { RealtimeAudit::freeAligned(value); }
void operator delete[](void* value, std::align_val_t) noexcept { RealtimeAudit::freeAligned(value); }
void operator delete(void* value, std::size_t, std::align_val_t) noexcept { RealtimeAudit::freeAligned(value); }
void operator delete[](void* value, std::size_t, std::align_val_t) noexcept { RealtimeAudit::freeAligned(value); }

using namespace Steinberg;
using namespace Steinberg::Vst;
namespace {
unsigned checks = 0, realtimeCalls = 0;
void require(bool condition, const char* message) {
  ++checks;
  if (!condition) {
    std::cerr << "FAIL: " << message << '\n';
    std::exit(1);
  }
}

class CheckedHost final : public HostApplication {
public:
  tresult PLUGIN_API createInstance(TUID cid, TUID iid, void** object) override {
    ++messageAllocations;
    if (RealtimeAudit::inside || std::this_thread::get_id() != uiThread) {
      ++forbiddenCalls;
      if (object) *object = nullptr;
      return kResultFalse;
    }
    return HostApplication::createInstance(cid, iid, object);
  }
  std::size_t messageAllocations = 0, forbiddenCalls = 0;
private:
  const std::thread::id uiThread = std::this_thread::get_id();
};

template<class Function>
void realtime(CheckedHost& host, Function&& callback) {
  const auto beforeNew = RealtimeAudit::allocations;
  const auto beforeMessages = host.messageAllocations;
  tresult result = kResultFalse;
  {
    RealtimeAudit::Scope scope;
    result = callback();
  }
  ++realtimeCalls;
  require(result == kResultOk, "realtime callback succeeds");
  require(RealtimeAudit::allocations == beforeNew, "no C++ allocation in realtime callback");
  require(host.messageAllocations == beforeMessages, "no host message allocation in realtime callback");
}

struct AudioFixture {
  AudioFixture(Meme::Processor& processor, CheckedHost& host) : processor(processor), host(host) {
    inputBus.numChannels = outputBus.numChannels = 2;
    inputBus.channelBuffers64 = sources;
    outputBus.channelBuffers64 = sinks;
    context.sampleRate = 48000;
    context.state = 0; // No kPlaying: this models live input with stopped transport.
    data.processMode = kRealtime;
    data.symbolicSampleSize = kSample64;
    data.numInputs = data.numOutputs = 1;
    data.inputs = &inputBus;
    data.outputs = &outputBus;
    data.processContext = &context;
    // Intentionally no host outputParameterChanges: the custom UI must still work.
  }
  void block(int samples = 4096) {
    for (int index = 0; index < samples; ++index, ++samplePosition) {
      left[index] = 0.5 * std::sin(2.0 * 3.14159265358979323846 * 1000.0 * samplePosition / 48000.0);
      right[index] = -left[index];
    }
    data.numSamples = samples;
    realtime(host, [&] { return processor.process(data); });
    require(std::memcmp(left.data(), outputLeft.data(), samples * sizeof(double)) == 0,
            "left audio remains bit identical");
    require(std::memcmp(right.data(), outputRight.data(), samples * sizeof(double)) == 0,
            "right audio remains bit identical");
  }
  Meme::Processor& processor;
  CheckedHost& host;
  std::array<double, 4096> left{}, right{}, outputLeft{}, outputRight{};
  double* sources[2] = {left.data(), right.data()};
  double* sinks[2] = {outputLeft.data(), outputRight.data()};
  AudioBusBuffers inputBus{}, outputBus{};
  ProcessContext context{};
  ProcessData data{};
  std::uint64_t samplePosition = 0;
};

void requireAllZero(const Meme::Controller& controller, const char* message) {
  for (int band = 0; band < 5; ++band) require(controller.meter(band) == 0, message);
}

IPtr<HostMessage> snapshot(bool omitAir = false) {
  auto message = owned(new HostMessage);
  message->setMessageID(Meme::meterSnapshot);
  auto* attributes = message->getAttributes();
  attributes->setInt("processing", 0);
  attributes->setInt("sequence", 123456);
  attributes->setFloat("idleGraceSeconds", 0.75);
  for (int band = 0; band < (omitAir ? 4 : 5); ++band)
    attributes->setFloat(Meme::meterKeys[band], 0.95);
  return message;
}
}

int main() {
  const auto started = std::chrono::steady_clock::now();
  CheckedHost host;
  auto processor = owned(new Meme::Processor);
  auto controller = owned(new Meme::Controller);
  require(processor->initialize(&host) == kResultOk, "processor initializes with SDK host");
  require(controller->initialize(&host) == kResultOk, "controller initializes with SDK host");
  require(processor->connect(controller) == kResultOk, "processor connects to controller");
  require(controller->connect(processor) == kResultOk, "controller connects to processor");
  ProcessSetup setup{};
  setup.processMode = kRealtime;
  setup.sampleRate = 48000;
  setup.maxSamplesPerBlock = 4096;
  setup.symbolicSampleSize = kSample64;
  require(processor->setupProcessing(setup) == kResultOk, "processor setup");
  require(processor->setActive(true) == kResultOk, "processor activation");
  realtime(host, [&] { return processor->setProcessing(true); });
  AudioFixture audio(*processor, host);
  for (int block = 0; block < 32; ++block) audio.block();
  controller->requestMeterSnapshot();
  const double heldMeter = controller->meter(2);
  require(heldMeter > 0.7, "real processor snapshot carries audible mid-band energy");
  require(host.messageAllocations >= 2, "UI request and response use actual SDK host messages");
  require((audio.context.state & ProcessContext::kPlaying) == 0, "transport remains stopped");

  // These nine small blocks cannot reach the next FFT (nine * 32 < 2048).
  // Liveness must follow callbacks, not changing FFT results or host parameters.
  const auto holdStarted = std::chrono::steady_clock::now();
  for (int block = 0; block < 9; ++block) {
    audio.block(32);
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    controller->requestMeterSnapshot();
    require(controller->meter(2) == heldMeter, "constant meter survives fresh short callbacks");
  }
  require(std::chrono::steady_clock::now() - holdStarted >= std::chrono::milliseconds(850),
          "held tone test exceeds idle grace");
  std::cout << "PASS: steady meter survives >850 ms with short callbacks and stopped-transport live input\n";

  realtime(host, [&] { return processor->setProcessing(false); });
  controller->requestMeterSnapshot();
  requireAllZero(*controller, "stopping immediately clears all snapshot meters without another block");
  for (int band = 0; band < 5; ++band)
    require(controller->setParamNormalized(Meme::meterBase + band, 1) == kResultOk, "late host parameter accepted");
  requireAllZero(*controller, "late host parameters cannot revive stopped meters");
  std::cout << "PASS: stop without another block clears all bands; late host parameters stay hidden\n";

  realtime(host, [&] { return processor->setProcessing(true); });
  audio.block(32);
  controller->requestMeterSnapshot();
  requireAllZero(*controller, "first short block after restart cannot resurrect previous FFT");
  for (int block = 0; block < 8; ++block) audio.block();
  controller->requestMeterSnapshot();
  require(controller->meter(2) > 0.7, "new input restores mid-band animation after restart");
  std::this_thread::sleep_for(std::chrono::milliseconds(850));
  controller->requestMeterSnapshot();
  requireAllZero(*controller, "suspended callbacks expire while processing flag stays true");
  audio.block(32);
  controller->requestMeterSnapshot();
  require(controller->meter(2) > 0.7, "fresh callback restores suspended animation");
  std::cout << "PASS: restart clears old FFT; suspended callbacks expire after 850 ms and resume on fresh input\n";

  const double validMeter = controller->meter(2);
  auto reject = [&](IPtr<HostMessage> malformed) {
    require(controller->notify(malformed) == kInvalidArgument, "malformed snapshot rejected");
    require(controller->meter(2) == validMeter, "malformed snapshot cannot replace valid display");
  };
  reject(snapshot(true));
  auto nonFinite = snapshot();
  nonFinite->getAttributes()->setFloat(Meme::meterKeys[0], std::numeric_limits<double>::infinity());
  reject(nonFinite);
  auto badState = snapshot();
  badState->getAttributes()->setInt("processing", 2);
  reject(badState);
  auto badGrace = snapshot();
  badGrace->getAttributes()->setFloat("idleGraceSeconds", std::numeric_limits<double>::quiet_NaN());
  reject(badGrace);
  auto shortGrace = snapshot();
  shortGrace->getAttributes()->setFloat("idleGraceSeconds", 0.1);
  reject(shortGrace);
  require(controller->notify(nullptr) == kInvalidArgument, "null snapshot rejected");
  require(controller->meter(2) == validMeter, "null snapshot preserves valid display");
  std::cout << "PASS: incomplete, non-finite, invalid-state, invalid-grace and null snapshots preserve valid state\n";

  require(controller->disconnect(processor) == kResultOk, "controller disconnect");
  require(processor->disconnect(controller) == kResultOk, "processor disconnect");
  requireAllZero(*controller, "disconnect clears stale snapshot and fallback values");
  require(controller->setParamNormalized(Meme::meterBase + 2, 0.42) == kResultOk, "fallback accepts host meter");
  require(controller->meter(2) == 0.42, "host meter fallback works after disconnect");
  realtime(host, [&] { return processor->setProcessing(false); });
  require(processor->setActive(false) == kResultOk, "processor deactivation");
  require(processor->terminate() == kResultOk, "processor termination");
  require(controller->terminate() == kResultOk, "controller termination");

  auto standalone = owned(new Meme::Controller);
  require(standalone->initialize(&host) == kResultOk, "standalone controller initialization");
  standalone->requestMeterSnapshot();
  for (double value : {std::numeric_limits<double>::quiet_NaN(),
                       std::numeric_limits<double>::infinity(),
                       -std::numeric_limits<double>::infinity()}) {
    require(standalone->setParamNormalized(Meme::meterBase, value) == kResultOk, "non-finite parameter sanitized");
    require(standalone->meter(0) == 0, "non-finite fallback renders zero");
    require(standalone->getParamNormalized(Meme::meterBase) == 0, "non-finite host parameter also stored as zero");
  }
  standalone->setParamNormalized(Meme::meterBase, 2);
  require(standalone->meter(0) == 1, "fallback clamps values above one");
  standalone->setParamNormalized(Meme::meterBase, -1);
  require(standalone->meter(0) == 0, "fallback clamps negative values");
  standalone->setParamNormalized(Meme::meterBase, 0.37);
  require(standalone->meter(0) == 0.37, "standalone host meter remains usable");
  require(standalone->terminate() == kResultOk, "standalone controller termination");
  require(host.forbiddenCalls == 0, "all host message allocation stayed on UI thread outside realtime callbacks");
  require(RealtimeAudit::allocations == 0, "zero C++ allocations across audited realtime callbacks");
  std::cout << "PASS: disconnect restores clean fallback; standalone NaN/infinity and range sanitization\n";
  std::cout << "PASS: " << realtimeCalls << " audited process/setProcessing calls, zero C++ allocations or host message calls; stereo audio bit identical\n";
  std::cout << "ALL METER LIFECYCLE TESTS PASSED: " << checks << " checks in "
            << std::chrono::duration<double>(std::chrono::steady_clock::now() - started).count()
            << " seconds\n";
}
