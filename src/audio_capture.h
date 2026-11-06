#pragma once

#include <atomic>
#include <string>
#include <thread>
#include <windows.h>

namespace streamdiscord
{
class AudioCapture
{
public:
    ~AudioCapture();

    void start(DWORD processId, std::wstring endpointId);
    void stop();

    HRESULT currentStatus() const
    {
        return captureStatus.load();
    }

private:
    void run(DWORD processId, const std::wstring& endpointId);

    std::thread worker;
    std::atomic<bool> stopping{false};
    std::atomic<HRESULT> captureStatus{S_FALSE};
};
} // namespace streamdiscord
