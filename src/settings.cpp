#include "settings.h"

#include <windows.h>

namespace streamdiscord
{
std::wstring settingsPath()
{
    wchar_t executablePath[32768];
    GetModuleFileNameW(nullptr, executablePath, 32768);

    std::wstring directory = executablePath;
    directory = directory.substr(0, directory.find_last_of(L'\\') + 1);
    return directory + L"sources.ini";
}

Settings loadSettings(const std::wstring& path)
{
    Settings settings;
    wchar_t value[1024];

    GetPrivateProfileStringW(L"Sources", L"Video", settings.videoExecutable.c_str(), value, 1024,
                             path.c_str());
    settings.videoExecutable = value;

    GetPrivateProfileStringW(L"Sources", L"Audio", L"", value, 1024, path.c_str());
    settings.audioExecutable = value;

    GetPrivateProfileStringW(L"Sources", L"Output", L"", value, 1024, path.c_str());
    settings.outputDeviceId = value;

    return settings;
}

void saveSettings(const std::wstring& path, const Settings& settings)
{
    WritePrivateProfileStringW(L"Sources", L"Video", settings.videoExecutable.c_str(),
                               path.c_str());
    WritePrivateProfileStringW(L"Sources", L"Audio", settings.audioExecutable.c_str(),
                               path.c_str());
    WritePrivateProfileStringW(L"Sources", L"Output", settings.outputDeviceId.c_str(),
                               path.c_str());
}
} // namespace streamdiscord
