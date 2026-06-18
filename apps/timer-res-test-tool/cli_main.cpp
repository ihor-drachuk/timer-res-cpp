/* License:  MIT
 * Source:   https://github.com/ihor-drachuk/timer-res-cpp
 * Contact:  ihor-drachuk-libs@pm.me  */

// cli_main.cpp — console front-end for the timer-resolution backend.
//
// Usage:
//   timer-res-test-tool                       same as `query`
//   timer-res-test-tool query                 print min / max / current
//   timer-res-test-tool set max|1ms|default|<100ns int>
//   timer-res-test-tool hold <value> <secs>   set, print current each second, release
//   timer-res-test-tool autostart on|off|status
//
// Exit codes: 0 ok, 1 usage error, 2 backend unavailable.
//
// IMPORTANT: on Windows 10 2004+ / 11 the timer resolution is per-process and
// is released when the requesting process exits. `set` therefore only affects
// the system while *some* process holds the request — running `set max` and
// returning has no lasting effect. Use `hold` (or the GUI, which stays
// resident) to actually keep a resolution applied.
//
// Note: C++23 std::print/println have no wchar_t overload, and the backend is
// UTF-16 throughout, so console output goes through a small WriteConsoleW
// helper (Console / ConsoleErr) instead.

#include <windows.h>

#include <array>
#include <cstdint>
#include <format>
#include <optional>
#include <string_view>

#include "autostart.h"
#include "timer_backend.h"

namespace {

// Write a UTF-16 string to a standard handle. Falls back to WriteFile when the
// handle is redirected to a pipe/file (where WriteConsoleW fails).
void WriteHandle(DWORD stdHandle, std::wstring_view text) {
    HANDLE h = GetStdHandle(stdHandle);
    if (h == INVALID_HANDLE_VALUE || h == nullptr) return;
    DWORD written = 0;
    if (!WriteConsoleW(h, text.data(), static_cast<DWORD>(text.size()),
                       &written, nullptr)) {
        WriteFile(h, text.data(),
                  static_cast<DWORD>(text.size() * sizeof(wchar_t)), &written,
                  nullptr);
    }
}

void Console(std::wstring_view text) { WriteHandle(STD_OUTPUT_HANDLE, text); }
void ConsoleErr(std::wstring_view text) { WriteHandle(STD_ERROR_HANDLE, text); }

// A parsed "set"/"hold" value: either an explicit 100-ns count or the
// "use the system's finest" request (the Maximum button's behavior).
struct DesiredValue {
    std::uint32_t value {};
    bool useMaximum {};
};

bool EqI(std::wstring_view a, std::wstring_view b) {
    return a.size() == b.size() && _wcsnicmp(a.data(), b.data(), a.size()) == 0;
}

// Parse a decimal uint32. Rejects empty input, non-digits, and overflow.
std::optional<std::uint32_t> ParseUInt(std::wstring_view s) {
    if (s.empty()) return std::nullopt;
    constexpr std::uint32_t kMax = 0xFFFFFFFFu;
    std::uint32_t v = 0;
    for (wchar_t c : s) {
        if (c < L'0' || c > L'9') return std::nullopt;
        const std::uint32_t d = static_cast<std::uint32_t>(c - L'0');
        if (v > (kMax - d) / 10) return std::nullopt;  // would overflow
        v = v * 10 + d;
    }
    return v;
}

std::optional<DesiredValue> ParseValue(std::wstring_view s) {
    if (EqI(s, L"max")) return DesiredValue{.useMaximum = true};
    if (EqI(s, L"1ms")) return DesiredValue{.value = timerres::kOneMs};
    if (EqI(s, L"default")) return DesiredValue{.value = timerres::kRelease};
    if (auto n = ParseUInt(s))
        return DesiredValue{.value = *n};
    return std::nullopt;
}

void Apply(const DesiredValue& d) {
    if (d.useMaximum)
        timerres::SetMaximum();
    else
        timerres::Set(d.value);
}

void PrintState() {
    const auto r = timerres::Query();
    if (!r) return;
    std::array<wchar_t, 32> a, b, c;
    timerres::FormatMs(r->minimum, a);
    timerres::FormatMs(r->maximum, b);
    timerres::FormatMs(r->current, c);
    Console(std::format(L"min={}  max={}  cur={}\n", a.data(), b.data(), c.data()));
}

int Usage() {
    ConsoleErr(
        L"usage: timer-res-test-tool [query | set <v> | hold <v> <secs> | "
        L"autostart on|off|status]\n"
        L"       v ::= max | 1ms | default | <100ns int>\n"
        L"note: timer resolution is per-process; `set` only holds while this\n"
        L"      process runs. Use `hold` or the GUI to keep it applied.\n");
    return 1;
}

}  // namespace

int wmain(int argc, wchar_t** argv) {
    if (!timerres::Init()) {
        ConsoleErr(L"ntdll timer-resolution exports unavailable\n");
        return 2;
    }
    const std::wstring_view cmd = argc >= 2 ? argv[1] : L"query";

    if (EqI(cmd, L"query")) {
        PrintState();
        return 0;
    }

    if (EqI(cmd, L"set")) {
        if (argc < 3) return Usage();
        const auto v = ParseValue(argv[2]);
        if (!v) return Usage();
        Apply(*v);
        PrintState();
        return 0;
    }

    if (EqI(cmd, L"hold")) {
        if (argc < 4) return Usage();
        const auto v = ParseValue(argv[2]);
        const auto secs = ParseUInt(argv[3]);
        if (!v || !secs) return Usage();
        Apply(*v);
        for (unsigned long i = 0; i < *secs; ++i) {
            PrintState();
            Sleep(1000);
        }
        timerres::Set(timerres::kRelease);
        Console(L"(released)\n");
        return 0;
    }

    if (EqI(cmd, L"autostart")) {
        const std::wstring_view sub = argc >= 3 ? argv[2] : L"status";
        if (EqI(sub, L"status")) {
            Console(autostart::IsEnabled() ? L"autostart: enabled\n" : L"autostart: disabled\n");
            return 0;
        }
        if (EqI(sub, L"on")) {
            Console(autostart::Enable() ? L"autostart: written\n" : L"autostart: FAILED\n");
            return 0;
        }
        if (EqI(sub, L"off")) {
            Console(autostart::Disable() ? L"autostart: removed\n" : L"autostart: FAILED\n");
            return 0;
        }
        return Usage();
    }

    return Usage();
}
