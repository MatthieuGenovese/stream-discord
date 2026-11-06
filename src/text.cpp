#include "text.h"
#include <windows.h>

namespace streamdiscord
{
std::string toUtf8(std::wstring const& value)
{
    if (value.empty())
    {
        return {};
    }
    int n = WideCharToMultiByte(CP_UTF8, 0, value.data(), static_cast<int>(value.size()), nullptr,
                                0, nullptr, nullptr);
    std::string result(n, '\0');
    WideCharToMultiByte(CP_UTF8, 0, value.data(), static_cast<int>(value.size()), result.data(), n,
                        nullptr, nullptr);
    return result;
}

std::wstring toWide(const char* value)
{
    int n = MultiByteToWideChar(CP_UTF8, 0, value, -1, nullptr, 0);
    if (n <= 1)
    {
        return {};
    }
    std::wstring result(n, L'\0');
    MultiByteToWideChar(CP_UTF8, 0, value, -1, result.data(), n);
    result.resize(n - 1);
    return result;
}

void copyUtf8(char* buffer, size_t capacity, std::wstring const& value)
{
    auto s = toUtf8(value);
    strncpy_s(buffer, capacity, s.c_str(), _TRUNCATE);
}

} // namespace streamdiscord
