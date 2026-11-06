#pragma once

#include <cstddef>
#include <string>

namespace streamdiscord
{
std::string toUtf8(const std::wstring& value);
std::wstring toWide(const char* value);
void copyUtf8(char* buffer, size_t capacity, const std::wstring& value);
} // namespace streamdiscord
