#pragma once
#include <algorithm>
#include <array>
#include <cmath>
#include <complex>
#include <cstdint>
#include <cstring>

namespace Meme {
// This analyzer reads a separate copy. The host's audio never passes through a filter.
class Analyzer {
public:
  static constexpr size_t size = 4096, hop = 2048;
  void prepare(double rate) noexcept {
    rate_ = rate;
    windowPower_ = 0;
    for (size_t n = 0; n < size; ++n) {
      window_[n] = 0.5 - 0.5 * std::cos(2 * pi * n / (size - 1));
      windowPower_ += window_[n] * window_[n];
      size_t x = n, reversed = 0;
      for (int bit = 0; bit < 12; ++bit) { reversed = (reversed << 1) | (x & 1); x >>= 1; }
      reverse_[n] = reversed;
    }
    for (size_t n = 0; n < size / 2; ++n)
      twiddle_[n] = {std::cos(-2 * pi * n / size), std::sin(-2 * pi * n / size)};
    attack_ = std::exp(-double(hop) / (rate_ * 0.045));
    release_ = std::exp(-double(hop) / (rate_ * 0.30));
    reset();
  }
  void reset() noexcept {
    for (auto& channel : ring_) channel.fill(0);
    levels_.fill(0); position_ = 0; untilFrame_ = hop; frames_ = 0;
  }
  template<class Sample>
  void feed(const Sample* const* input, int channels, int samples, uint64_t silence = 0) noexcept {
    if (channels < 1 || channels > 2) return;
    for (int sample = 0; sample < samples; ++sample) {
      for (int channel = 0; channel < channels; ++channel) {
        double value = ((silence >> channel) & 1) || !input[channel] ? 0 : double(input[channel][sample]);
        // A hostile/non-finite signal must not contaminate the graphics. Audio remains untouched.
        ring_[channel][position_] = std::isfinite(value) ? std::clamp(value, -64.0, 64.0) : 0;
      }
      position_ = (position_ + 1) % size;
      if (--untilFrame_ == 0) { analyze(channels); untilFrame_ = hop; ++frames_; }
    }
  }
  const std::array<double, 5>& levels() const noexcept { return levels_; }
  uint64_t frames() const noexcept { return frames_; }
private:
  void analyze(int channels) noexcept {
    std::array<double, 5> power{};
    for (int channel = 0; channel < channels; ++channel) {
      for (size_t n = 0; n < size; ++n) fft_[reverse_[n]] = ring_[channel][(position_ + n) % size] * window_[n];
      for (size_t length = 2; length <= size; length <<= 1) {
        const size_t half = length / 2, stride = size / length;
        for (size_t start = 0; start < size; start += length)
          for (size_t j = 0; j < half; ++j) {
            const auto odd = fft_[start + j + half] * twiddle_[j * stride];
            const auto even = fft_[start + j];
            fft_[start + j] = even + odd; fft_[start + j + half] = even - odd;
          }
      }
      for (size_t bin = 0; bin <= size / 2; ++bin) {
        const double hz = double(bin) * rate_ / size;
        if (hz > 20000) continue;
        const size_t band = hz < 100 ? 0 : hz < 500 ? 1 : hz < 2000 ? 2 : hz < 6000 ? 3 : 4;
        const double weight = bin == 0 || bin == size / 2 ? 1 : 2;
        power[band] += weight * std::norm(fft_[bin]) / (size * windowPower_ * channels);
      }
    }
    for (size_t band = 0; band < 5; ++band) {
      const double rms = std::sqrt(power[band]);
      const double coefficient = rms > levels_[band] ? attack_ : release_;
      levels_[band] = coefficient * levels_[band] + (1 - coefficient) * rms;
      if (levels_[band] < 1e-12) levels_[band] = 0;
    }
  }
  static constexpr double pi = 3.14159265358979323846;
  double rate_ = 48000, windowPower_ = 1, attack_ = 0, release_ = 0;
  std::array<std::array<double, size>, 2> ring_{};
  std::array<double, size> window_{};
  std::array<size_t, size> reverse_{};
  std::array<std::complex<double>, size> fft_{};
  std::array<std::complex<double>, size / 2> twiddle_{};
  std::array<double, 5> levels_{};
  size_t position_ = 0, untilFrame_ = hop;
  uint64_t frames_ = 0;
};
template<class Sample>
void copyAudio(const Sample* const* input, Sample* const* output, int channels, int samples, uint64_t silence) noexcept {
  for (int c = 0; c < channels; ++c) {
    if (!output[c]) continue;
    if (((silence >> c) & 1) || !input[c]) std::memset(output[c], 0, sizeof(Sample) * samples);
    else if (input[c] != output[c]) std::memmove(output[c], input[c], sizeof(Sample) * samples);
  }
}
}
