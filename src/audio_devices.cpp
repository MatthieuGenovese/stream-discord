#include "audio_devices.h"
#include "common.h"
#include <mmdeviceapi.h>
#include <propkeydef.h>
#include <wrl/client.h>

#include <functiondiscoverykeys_devpkey.h>

namespace streamdiscord
{
using Microsoft::WRL::ComPtr;
using namespace winrt;
std::vector<OutputDevice> enumerateAudioOutputs()
{
    std::vector<OutputDevice> available;
    ComPtr<IMMDeviceEnumerator> enumerator;
    check(CoCreateInstance(__uuidof(MMDeviceEnumerator), nullptr, CLSCTX_ALL,
                           IID_PPV_ARGS(&enumerator)));
    ComPtr<IMMDevice> defaultDevice;
    std::wstring defaultId;
    LPWSTR id = nullptr;
    if (SUCCEEDED(enumerator->GetDefaultAudioEndpoint(eRender, eMultimedia, &defaultDevice)) &&
        SUCCEEDED(defaultDevice->GetId(&id)))
    {
        defaultId = id;
        CoTaskMemFree(id);
    }
    ComPtr<IMMDeviceCollection> collection;
    check(enumerator->EnumAudioEndpoints(eRender, DEVICE_STATE_ACTIVE, &collection));
    UINT count;
    check(collection->GetCount(&count));

    for (UINT i = 0; i < count; ++i)
    {
        ComPtr<IMMDevice> device;
        check(collection->Item(i, &device));
        check(device->GetId(&id));
        std::wstring deviceId = id;
        CoTaskMemFree(id);
        ComPtr<IPropertyStore> properties;
        check(device->OpenPropertyStore(STGM_READ, &properties));
        PROPVARIANT name{};
        check(properties->GetValue(PKEY_Device_FriendlyName, &name));
        std::wstring label = name.vt == VT_LPWSTR ? name.pwszVal : L"Périphérique audio";
        PropVariantClear(&name);
        available.push_back({deviceId, label, deviceId == defaultId});
    }
    return available;
}
} // namespace streamdiscord
