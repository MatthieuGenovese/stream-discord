#pragma once

#include <d3d11.h>
#include <dxgi1_2.h>
#include <winrt/Windows.Graphics.Capture.h>
#include <winrt/Windows.Graphics.DirectX.Direct3D11.h>
#include <wrl/client.h>

namespace streamdiscord
{
class VideoCapture
{
public:
    ~VideoCapture();

    void start(HWND source, HWND target);
    void stop();
    void tick();

private:
    Microsoft::WRL::ComPtr<ID3D11Device> device;
    Microsoft::WRL::ComPtr<ID3D11DeviceContext> context;
    Microsoft::WRL::ComPtr<IDXGISwapChain1> swapChain;
    winrt::Windows::Graphics::DirectX::Direct3D11::IDirect3DDevice captureDevice{nullptr};
    winrt::Windows::Graphics::Capture::GraphicsCaptureItem captureItem{nullptr};
    winrt::Windows::Graphics::Capture::Direct3D11CaptureFramePool framePool{nullptr};
    winrt::Windows::Graphics::Capture::GraphicsCaptureSession captureSession{nullptr};
    int width = 0;
    int height = 0;
};
} // namespace streamdiscord
