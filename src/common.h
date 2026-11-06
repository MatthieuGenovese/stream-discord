#pragma once

#include <windows.h>
#include <winrt/base.h>

namespace streamdiscord
{
inline void check(HRESULT result)
{
    winrt::check_hresult(result);
}
} // namespace streamdiscord
