#pragma once

namespace Meme {
// Versioned, value-only messages also work through a host connection proxy.
inline constexpr const char* meterRequest = "VaginaPlugin.Meters.Request.v1";
inline constexpr const char* meterSnapshot = "VaginaPlugin.Meters.Snapshot.v1";
inline constexpr const char* meterKeys[] = {"sub", "bass", "mid", "high", "air"};
}
