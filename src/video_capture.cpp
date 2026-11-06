#include "video_capture.h"
#include "common.h"
#include <windows.graphics.capture.interop.h>
#include <windows.graphics.directx.direct3d11.interop.h>
#include <winrt/Windows.Foundation.h>

namespace streamdiscord
{
using Microsoft::WRL::ComPtr;
using namespace winrt;

using namespace winrt::Windows::Graphics::Capture;
using namespace winrt::Windows::Graphics::DirectX;
using namespace winrt::Windows::Graphics::DirectX::Direct3D11;
void VideoCapture::stop()
{
    if (captureSession)
    {
        captureSession.Close();
    }
    if (framePool)
    {
        framePool.Close();
    }
    captureSession = nullptr;
    framePool = nullptr;
    captureItem = nullptr;
    swapChain.Reset();
}

void VideoCapture::start(HWND source, HWND target)
{
    stop();
    UINT flags = D3D11_CREATE_DEVICE_BGRA_SUPPORT;
    D3D_FEATURE_LEVEL feature;
    check(D3D11CreateDevice(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, flags, nullptr, 0,
                            D3D11_SDK_VERSION, &device, &feature, &context));
    ComPtr<IDXGIDevice> dxgi;
    check(device.As(&dxgi));
    com_ptr<IInspectable> inspectable;
    check(CreateDirect3D11DeviceFromDXGIDevice(dxgi.Get(), inspectable.put()));
    captureDevice = inspectable.as<IDirect3DDevice>();
    auto interop = get_activation_factory<GraphicsCaptureItem, IGraphicsCaptureItemInterop>();
    check(interop->CreateForWindow(source, guid_of<GraphicsCaptureItem>(), put_abi(captureItem)));
    width = captureItem.Size().Width;
    height = captureItem.Size().Height;
    ComPtr<IDXGIAdapter> adapter;
    check(dxgi->GetAdapter(&adapter));
    ComPtr<IDXGIFactory2> factory;
    check(adapter->GetParent(IID_PPV_ARGS(&factory)));
    DXGI_SWAP_CHAIN_DESC1 description{};
    description.Width = width;
    description.Height = height;
    description.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
    description.SampleDesc.Count = 1;
    description.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    description.BufferCount = 2;
    description.SwapEffect = DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL;
    description.Scaling = DXGI_SCALING_STRETCH;
    check(factory->CreateSwapChainForHwnd(device.Get(), target, &description, nullptr, nullptr,
                                          &swapChain));
    framePool = Direct3D11CaptureFramePool::CreateFreeThreaded(
        captureDevice, DirectXPixelFormat::B8G8R8A8UIntNormalized, 2, captureItem.Size());
    captureSession = framePool.CreateCaptureSession(captureItem);
    captureSession.IsCursorCaptureEnabled(false);
    captureSession.StartCapture();
}

void VideoCapture::tick()
{
    if (!framePool)
    {
        return;
    }
    auto frame = framePool.TryGetNextFrame();
    if (!frame)
    {
        return;
    }
    auto size = frame.ContentSize();
    if (size.Width <= 0 || size.Height <= 0)
    {
        return;
    }
    if (size.Width != width || size.Height != height)
    {
        frame.Close();
        width = size.Width;
        height = size.Height;
        check(swapChain->ResizeBuffers(2, width, height, DXGI_FORMAT_UNKNOWN, 0));
        framePool.Recreate(captureDevice, DirectXPixelFormat::B8G8R8A8UIntNormalized, 2, size);
        return;
    }
    auto access = frame.Surface()
                      .as<::Windows::Graphics::DirectX::Direct3D11::IDirect3DDxgiInterfaceAccess>();
    ComPtr<ID3D11Texture2D> texture, back;
    check(access->GetInterface(IID_PPV_ARGS(&texture)));
    check(swapChain->GetBuffer(0, IID_PPV_ARGS(&back)));
    context->CopyResource(back.Get(), texture.Get());
    check(swapChain->Present(0, 0));
}

VideoCapture::~VideoCapture()
{
    stop();
}
} // namespace streamdiscord
