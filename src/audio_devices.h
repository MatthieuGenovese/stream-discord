#pragma once

#include <string>
#include <vector>

namespace streamdiscord
{
struct OutputDevice
{
    std::wstring id;
    std::wstring name;
    bool isDefault;
};

std::vector<OutputDevice> enumerateAudioOutputs();
} // namespace streamdiscord
