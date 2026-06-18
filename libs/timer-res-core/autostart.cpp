/* License:  MIT
 * Source:   https://github.com/ihor-drachuk/timer-res-cpp
 * Contact:  ihor-drachuk-libs@pm.me  */

#include "autostart.h"

#include <windows.h>

#include <optional>
#include <string>

namespace autostart {
namespace {

constexpr std::wstring_view kRunKey =
    L"Software\\Microsoft\\Windows\\CurrentVersion\\Run";

// RAII wrapper around an open registry key.
class RegKey {
public:
    RegKey(HKEY root, std::wstring_view sub, REGSAM access) {
        if (RegOpenKeyExW(root, sub.data(), 0, access, &key_) != ERROR_SUCCESS)
            key_ = nullptr;
    }
    ~RegKey() {
        if (key_) RegCloseKey(key_);
    }
    RegKey(const RegKey&) = delete;
    RegKey& operator=(const RegKey&) = delete;

    [[nodiscard]] bool valid() const noexcept { return key_ != nullptr; }
    [[nodiscard]] HKEY get() const noexcept { return key_; }

private:
    HKEY key_ {};
};

// Full path to this executable. Grows the buffer until it fits, so paths
// longer than MAX_PATH (long-path-aware processes) aren't silently truncated.
// Returns std::nullopt on failure.
[[nodiscard]] std::optional<std::wstring> ExePath() {
    std::wstring path(MAX_PATH, L'\0');
    for (;;) {
        const DWORD n = GetModuleFileNameW(nullptr, path.data(),
                                           static_cast<DWORD>(path.size()));
        if (n == 0) return std::nullopt;            // hard failure
        if (n < path.size()) {                      // fit (n excludes the NUL)
            path.resize(n);
            return path;
        }
        if (path.size() > 0x8000) return std::nullopt;  // sanity cap (~32K)
        path.resize(path.size() * 2);               // truncated -> grow & retry
    }
}

// Full path to this executable, quoted, with " -min" appended. std::nullopt on
// failure.
[[nodiscard]] std::optional<std::wstring> BuildCommand() {
    const auto exe = ExePath();
    if (!exe) return std::nullopt;
    std::wstring cmd;
    cmd.reserve(exe->size() + 8);
    cmd += L'"';
    cmd += *exe;
    cmd += L"\" -min";
    return cmd;
}

}  // namespace

bool IsEnabled() {
    const RegKey key{HKEY_CURRENT_USER, kRunKey, KEY_READ};
    if (!key.valid()) return false;
    return RegQueryValueExW(key.get(), kValueName, nullptr, nullptr, nullptr,
                            nullptr) == ERROR_SUCCESS;
}

bool Enable() {
    const auto cmd = BuildCommand();
    if (!cmd) return false;  // couldn't resolve our own path
    const RegKey key{HKEY_CURRENT_USER, kRunKey, KEY_SET_VALUE};
    if (!key.valid()) return false;
    const auto bytes = static_cast<DWORD>((cmd->size() + 1) * sizeof(wchar_t));
    return RegSetValueExW(key.get(), kValueName, 0, REG_SZ,
                          reinterpret_cast<const BYTE*>(cmd->c_str()),
                          bytes) == ERROR_SUCCESS;
}

bool Disable() {
    const RegKey key{HKEY_CURRENT_USER, kRunKey, KEY_SET_VALUE};
    if (!key.valid()) return true;  // key missing -> nothing to remove
    const LONG rc = RegDeleteValueW(key.get(), kValueName);
    return rc == ERROR_SUCCESS || rc == ERROR_FILE_NOT_FOUND;
}

}  // namespace autostart
