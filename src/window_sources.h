#pragma once

#include <string>
#include <vector>
#include <windows.h>

namespace streamdiscord
{
struct WindowSource
{
    HWND hwnd;
    DWORD pid;
    std::wstring exe;
    std::wstring title;
};

std::wstring executableName(DWORD processId);
std::wstring lowercase(std::wstring value);
std::vector<WindowSource> enumerateWindows();
} // namespace streamdiscord
