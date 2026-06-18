/* License:  MIT
 * Source:   https://github.com/ihor-drachuk/timer-res-cpp
 * Contact:  ihor-drachuk-libs@pm.me  */

// autostart.h — manage a HKCU\…\Run entry so the app launches at logon.
// No admin rights required (HKCU, not HKLM). Shared by CLI and GUI.
#pragma once

namespace autostart {

// Registry value name under HKCU\Software\Microsoft\Windows\CurrentVersion\Run.
inline constexpr wchar_t kValueName[] = L"TimerRes";

// True if our Run value currently exists.
bool IsEnabled();

// Writes `"<full exe path>" -min` to the Run key. Returns true on success.
// The "-min" flag tells the app to start hidden (tray only).
bool Enable();

// Removes the Run value. Returns true on success (or if it was already absent).
bool Disable();

}  // namespace autostart
