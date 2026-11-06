#pragma once
#include <d3d11.h>
#include <dxgi.h>
#include <wrl/client.h>

namespace streamdiscord
{
class InterfaceRenderer
{
public:
    void init(HWND window);
    void resize(UINT width, UINT height);
    void begin();
    void end();
    void shutdown();
    float dpiScale() const
    {
        return scale;
    }

private:
    void createTarget();
    Microsoft::WRL::ComPtr<ID3D11Device> device;
    Microsoft::WRL::ComPtr<ID3D11DeviceContext> context;
    Microsoft::WRL::ComPtr<IDXGISwapChain> swap;
    Microsoft::WRL::ComPtr<ID3D11RenderTargetView> target;
    bool initialized = false;
    float scale = 1.0f;
};
} // namespace streamdiscord
