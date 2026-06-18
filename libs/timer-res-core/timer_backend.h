/* License:  MIT
 * Source:   https://github.com/ihor-drachuk/timer-res-cpp
 * Contact:  ihor-drachuk-libs@pm.me  */

// timer_backend.h — Windows timer-resolution control via undocumented ntdll
// exports (NtQueryTimerResolution / NtSetTimerResolution). Shared by the CLI
// and GUI front-ends.
//
// All resolution values are in 100-nanosecond units:
//   10000  = 1 ms
//    5000  = 0.5 ms
//  156250  ~ 15.625 ms  (typical Windows default)
#pragma once

#include <cstdint>
#include <optional>
#include <span>

namespace timerres {

// Snapshot of the system timer-resolution range plus the active value.
struct Resolution {
    std::uint32_t minimum {};   // coarsest supported (largest number, e.g. 156250)
    std::uint32_t maximum {};   // finest supported   (smallest number, e.g. 5000)
    std::uint32_t current {};   // currently active
};

// Well-known desired values (100-ns units). Pass 0 to release.
inline constexpr std::uint32_t kRelease = 0;
inline constexpr std::uint32_t kOneMs   = 10000;   // 1 ms
inline constexpr std::uint32_t kFourMs  = 40000;   // 4 ms

// Resolves the ntdll entry points. Returns false if they are unavailable
// (should never happen on a real Windows install). Safe to call repeatedly.
bool Init();

// Queries the current min/max/current, or std::nullopt if ntdll wasn't resolved.
[[nodiscard]] std::optional<Resolution> Query();

// Sets the timer resolution to `desired` (100-ns units). Pass kRelease (0) to
// release the request. Returns the actual rounded value the kernel applied, or
// std::nullopt if ntdll wasn't resolved.
std::optional<std::uint32_t> Set(std::uint32_t desired);

// Convenience: set to the finest value the system reports (its `maximum`).
// Returns the applied value, or std::nullopt on failure.
std::optional<std::uint32_t> SetMaximum();

// Formats a 100-ns value as "<u>.<u04> ms" into `buf` (UTF-16). Returns the
// number of chars written, or -1 on truncation. Needs <= 18 wchars for the
// widest value; the 32-wchar buffers callers use never truncate.
int FormatMs(std::uint32_t value, std::span<wchar_t> buf);

}  // namespace timerres
