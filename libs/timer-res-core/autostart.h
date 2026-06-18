/* License:  MIT
 * Source:   https://github.com/ihor-drachuk/timer-res-cpp
 * Contact:  ihor-drachuk-libs@pm.me  */

// autostart.h — manage a HKCU\…\Run entry so the app launches at logon, plus a
// small store for the last-applied timer value so a logon launch can resume it.
// No admin rights required (HKCU, not HKLM). Shared by CLI and GUI.
#pragma once

#include <cstdint>

namespace autostart {

// Registry value name under HKCU\Software\Microsoft\Windows\CurrentVersion\Run.
inline constexpr wchar_t kValueName[] = L"TimerRes";

// True if our Run value currently exists.
bool IsEnabled();

// Writes `"<full exe path>" --min --resume` to the Run key. Returns true on
// success. "--min" starts hidden (tray only); "--resume" re-applies the last
// stored timer value (see StoreLastValue / LoadLastValue).
bool Enable();

// Removes the Run value. Returns true on success (or if it was already absent).
bool Disable();

// Persist the last-applied timer value (100-ns units) under
// HKCU\Software\timer-res\LastValue. A value of 0 (release) deletes the entry,
// so a later LoadLastValue / --resume becomes a no-op.
void StoreLastValue(std::uint32_t value);

// Read the last-applied timer value (100-ns units). Returns 0 if absent.
std::uint32_t LoadLastValue();

}  // namespace autostart
