#pragma once

#include <string>

namespace streamdiscord
{
struct Settings
{
    std::wstring videoExecutable = L"League of Legends.exe";
    std::wstring audioExecutable;
    std::wstring outputDeviceId;
};

std::wstring settingsPath();
Settings loadSettings(const std::wstring& path);
void saveSettings(const std::wstring& path, const Settings& settings);
} // namespace streamdiscord
