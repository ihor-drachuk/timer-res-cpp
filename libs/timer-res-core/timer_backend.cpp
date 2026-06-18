/* License:  MIT
 * Source:   https://github.com/ihor-drachuk/timer-res-cpp
 * Contact:  ihor-drachuk-libs@pm.me  */

#include "timer_backend.h"

#include <windows.h>

#include <cstdio>

namespace timerres {
namespace {

using NtQueryFn = LONG(NTAPI*)(PULONG MinimumResolution,
                               PULONG MaximumResolution,
                               PULONG CurrentResolution);
using NtSetFn   = LONG(NTAPI*)(ULONG DesiredResolution,
                               BOOLEAN SetResolution,
                               PULONG CurrentResolution);

NtQueryFn g_query = nullptr;
NtSetFn   g_set   = nullptr;

}  // namespace

bool Init() {
    if (g_query && g_set) return true;
    HMODULE ntdll = GetModuleHandleW(L"ntdll.dll");
    if (!ntdll) return false;
    g_query = reinterpret_cast<NtQueryFn>(
        reinterpret_cast<void*>(GetProcAddress(ntdll, "NtQueryTimerResolution")));
    g_set = reinterpret_cast<NtSetFn>(
        reinterpret_cast<void*>(GetProcAddress(ntdll, "NtSetTimerResolution")));
    return g_query && g_set;
}

// NTSTATUS success is status >= 0 (the high bit is the severity sign bit).
constexpr bool NtSuccess(LONG status) { return status >= 0; }

std::optional<Resolution> Query() {
    if (!g_query) return std::nullopt;
    ULONG mn = 0, mx = 0, cur = 0;
    if (!NtSuccess(g_query(&mn, &mx, &cur))) return std::nullopt;
    return Resolution{.minimum = mn, .maximum = mx, .current = cur};
}

std::optional<std::uint32_t> Set(std::uint32_t desired) {
    if (!g_set) return std::nullopt;
    ULONG cur = 0;
    const BOOLEAN setIt = desired != 0 ? TRUE : FALSE;
    if (!NtSuccess(g_set(static_cast<ULONG>(desired), setIt, &cur)))
        return std::nullopt;
    return cur;
}

std::optional<std::uint32_t> SetMaximum() {
    const auto r = Query();
    if (!r) return std::nullopt;
    return Set(r->maximum);
}

int FormatMs(std::uint32_t value, std::span<wchar_t> buf) {
    const unsigned whole = value / 10000u;
    const unsigned rem   = value % 10000u;
    return _snwprintf_s(buf.data(), buf.size(), _TRUNCATE, L"%u.%04u ms", whole,
                        rem);
}

}  // namespace timerres
