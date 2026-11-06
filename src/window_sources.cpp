#include "window_sources.h"
#include <algorithm>
#include <cwctype>

namespace streamdiscord
{
std::wstring windowTitle(HWND h)
{
    int n = GetWindowTextLengthW(h);
    std::wstring s(n + 1, L'\0');
    GetWindowTextW(h, s.data(), n + 1);
    s.resize(n);
    return s;
}

std::wstring lowercase(std::wstring s)
{
    std::transform(s.begin(), s.end(), s.begin(), towlower);
    return s;
}

std::wstring executableName(DWORD pid)
{
    wchar_t p[32768];
    DWORD n = 32768;
    HANDLE h = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pid);
    if (!h)
    {
        return {};
    }
    bool ok = QueryFullProcessImageNameW(h, 0, p, &n);
    CloseHandle(h);
    if (!ok)
    {
        return {};
    }
    std::wstring s(p, n);
    return s.substr(s.find_last_of(L"\\/") + 1);
}

std::vector<WindowSource> enumerateWindows()
{
    std::vector<WindowSource> r;
    EnumWindows(
        [](HWND h, LPARAM p) -> BOOL
        {
            if (!IsWindowVisible(h) || GetWindow(h, GW_OWNER) || GetWindowTextLengthW(h) == 0)
            {
                return TRUE;
            }
            DWORD pid;
            GetWindowThreadProcessId(h, &pid);
            if (pid == GetCurrentProcessId())
            {
                return TRUE;
            }
            auto exe = executableName(pid);
            if (!exe.empty())
            {
                reinterpret_cast<std::vector<WindowSource>*>(p)->push_back(
                    {h, pid, exe, windowTitle(h)});
            }
            return TRUE;
        },
        reinterpret_cast<LPARAM>(&r));
    return r;
}

} // namespace streamdiscord
