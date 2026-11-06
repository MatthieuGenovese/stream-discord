#include "audio_capture.h"
#include "common.h"
#include <algorithm>
#include <audioclient.h>
#include <audioclientactivationparams.h>
#include <deque>
#include <mmdeviceapi.h>
#include <wrl.h>

namespace streamdiscord
{
using Microsoft::WRL::ComPtr;
using namespace winrt;
namespace
{
class Activation final : public Microsoft::WRL::RuntimeClass<
                             Microsoft::WRL::RuntimeClassFlags<Microsoft::WRL::ClassicCom>,
                             IActivateAudioInterfaceCompletionHandler, Microsoft::WRL::FtmBase>
{
public:
    HANDLE done = CreateEventW(nullptr, TRUE, FALSE, nullptr);
    HRESULT result = E_PENDING;
    ComPtr<IAudioClient> client;
    ~Activation()
    {
        CloseHandle(done);
    }
    STDMETHOD(ActivateCompleted)(IActivateAudioInterfaceAsyncOperation* op) override
    {
        ComPtr<IUnknown> value;
        HRESULT hr;
        result = op->GetActivateResult(&hr, &value);
        if (SUCCEEDED(result))
        {
            result = hr;
        }
        if (SUCCEEDED(result))
        {
            result = value.As(&client);
        }
        SetEvent(done);
        return S_OK;
    }
};

} // namespace
void AudioCapture::stop()
{
    stopping = true;
    if (worker.joinable())
    {
        worker.join();
    }
    captureStatus = S_FALSE;
}

void AudioCapture::start(DWORD processId, std::wstring endpointId)
{
    stop();
    stopping = false;
    captureStatus = E_PENDING;
    worker = std::thread(
        [this, processId, endpointId]
        {
            run(processId, endpointId);
        });
}

void AudioCapture::run(DWORD processId, const std::wstring& endpointId)
{
    HRESULT init = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
    if (FAILED(init))
    {
        captureStatus = init;
        return;
    }
    try
    {
        auto callback = Microsoft::WRL::Make<Activation>();
        AUDIOCLIENT_ACTIVATION_PARAMS params{};
        params.ActivationType = AUDIOCLIENT_ACTIVATION_TYPE_PROCESS_LOOPBACK;
        params.ProcessLoopbackParams.TargetProcessId = processId;
        params.ProcessLoopbackParams.ProcessLoopbackMode =
            PROCESS_LOOPBACK_MODE_INCLUDE_TARGET_PROCESS_TREE;
        PROPVARIANT prop{};
        prop.vt = VT_BLOB;
        prop.blob.cbSize = sizeof(params);
        prop.blob.pBlobData = reinterpret_cast<BYTE*>(&params);
        ComPtr<IActivateAudioInterfaceAsyncOperation> op;
        check(ActivateAudioInterfaceAsync(VIRTUAL_AUDIO_DEVICE_PROCESS_LOOPBACK,
                                          __uuidof(IAudioClient), &prop, callback.Get(), &op));
        int elapsed = 0;
        while (WaitForSingleObject(callback->done, 50) == WAIT_TIMEOUT)
        {
            if (stopping)
            {
                CoUninitialize();
                return;
            }
            if ((elapsed += 50) >= 10000)
            {
                throw hresult_error(HRESULT_FROM_WIN32(ERROR_TIMEOUT));
            }
        }
        check(callback->result);
        auto captureClient = callback->client;
        WAVEFORMATEX format{};
        format.wFormatTag = WAVE_FORMAT_IEEE_FLOAT;
        format.nChannels = 2;
        format.nSamplesPerSec = 48000;
        format.wBitsPerSample = 32;
        format.nBlockAlign = 8;
        format.nAvgBytesPerSec = 384000;
        check(captureClient->Initialize(AUDCLNT_SHAREMODE_SHARED,
                                        AUDCLNT_STREAMFLAGS_LOOPBACK |
                                            AUDCLNT_STREAMFLAGS_AUTOCONVERTPCM,
                                        200000, 0, &format, nullptr));
        ComPtr<IAudioCaptureClient> capture;
        check(captureClient->GetService(IID_PPV_ARGS(&capture)));
        ComPtr<IMMDeviceEnumerator> enumerator;
        check(CoCreateInstance(__uuidof(MMDeviceEnumerator), nullptr, CLSCTX_ALL,
                               IID_PPV_ARGS(&enumerator)));
        ComPtr<IMMDevice> endpoint;
        check(enumerator->GetDevice(endpointId.c_str(), &endpoint));
        ComPtr<IAudioClient> output;
        check(endpoint->Activate(__uuidof(IAudioClient), CLSCTX_ALL, nullptr, &output));
        check(output->Initialize(AUDCLNT_SHAREMODE_SHARED,
                                 AUDCLNT_STREAMFLAGS_AUTOCONVERTPCM |
                                     AUDCLNT_STREAMFLAGS_SRC_DEFAULT_QUALITY,
                                 300000, 0, &format, nullptr));
        ComPtr<IAudioRenderClient> render;
        check(output->GetService(IID_PPV_ARGS(&render)));
        UINT32 capacity;
        check(output->GetBufferSize(&capacity));
        check(output->Start());
        check(captureClient->Start());
        captureStatus = S_OK;
        std::deque<float> queue;
        while (!stopping)
        {
            UINT32 packets = 0;
            check(capture->GetNextPacketSize(&packets));
            while (packets)
            {
                BYTE* data;
                UINT32 frames;
                DWORD flags;
                check(capture->GetBuffer(&data, &frames, &flags, nullptr, nullptr));
                for (UINT32 i = 0; i < frames * 2; ++i)
                {
                    queue.push_back(flags & AUDCLNT_BUFFERFLAGS_SILENT
                                        ? 0.0f
                                        : reinterpret_cast<float*>(data)[i]);
                }
                check(capture->ReleaseBuffer(frames));
                check(capture->GetNextPacketSize(&packets));
            }
            while (queue.size() > 48000 / 5 * 2)
            {
                queue.pop_front();
            }
            UINT32 padding;
            check(output->GetCurrentPadding(&padding));
            UINT32 count = std::min(capacity - padding, static_cast<UINT32>(queue.size() / 2));
            if (count)
            {
                BYTE* data;
                check(render->GetBuffer(count, &data));
                for (UINT32 i = 0; i < count * 2; ++i)
                {
                    reinterpret_cast<float*>(data)[i] = queue.front();
                    queue.pop_front();
                }
                check(render->ReleaseBuffer(count, 0));
            }
            Sleep(4);
        }
        captureClient->Stop();
        output->Stop();
    }
    catch (hresult_error const& e)
    {
        captureStatus = e.code();
    }
    catch (...)
    {
        captureStatus = E_FAIL;
    }
    CoUninitialize();
}

AudioCapture::~AudioCapture()
{
    stop();
}
} // namespace streamdiscord
