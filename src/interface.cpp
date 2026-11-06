#include "interface.h"
#include "backends/imgui_impl_dx11.h"
#include "backends/imgui_impl_win32.h"
#include "common.h"
#include "imgui.h"
#include "text.h"
#include <dwmapi.h>

namespace streamdiscord
{
using Microsoft::WRL::ComPtr;
using namespace winrt;

void InterfaceRenderer::createTarget()
{
    ComPtr<ID3D11Texture2D> buffer;
    check(swap->GetBuffer(0, IID_PPV_ARGS(&buffer)));
    check(device->CreateRenderTargetView(buffer.Get(), nullptr, &target));
}

void InterfaceRenderer::init(HWND window)
{
    DXGI_SWAP_CHAIN_DESC desc{};
    desc.BufferCount = 2;
    desc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    desc.OutputWindow = window;
    desc.SampleDesc.Count = 1;
    desc.Windowed = TRUE;
    desc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
    D3D_FEATURE_LEVEL level;
    check(D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, 0, nullptr, 0,
                                        D3D11_SDK_VERSION, &desc, &swap, &device, &level,
                                        &context));
    createTarget();
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    auto& io = ImGui::GetIO();
    io.IniFilename = nullptr;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    scale = ImGui_ImplWin32_GetDpiScaleForHwnd(window);
    wchar_t directory[MAX_PATH];
    GetWindowsDirectoryW(directory, MAX_PATH);
    std::wstring fontPath = std::wstring(directory) + L"\\Fonts\\segoeui.ttf";
    std::string fontUtf8 = toUtf8(fontPath);
    static const ImWchar glyphs[] = {0x0020, 0x00FF, 0x2000, 0x206F, 0};
    if (!io.Fonts->AddFontFromFileTTF(fontUtf8.c_str(), 18.0f * scale, nullptr, glyphs))
    {
        io.Fonts->AddFontDefault();
    }
    auto& style = ImGui::GetStyle();
    ImGui::StyleColorsDark();
    style.WindowRounding = 0;
    style.ChildRounding = 14;
    style.FrameRounding = 7;
    style.PopupRounding = 9;
    style.WindowBorderSize = 0;
    style.ChildBorderSize = 1;
    style.FrameBorderSize = 1;
    style.WindowPadding = {28, 24};
    style.FramePadding = {12, 10};
    style.ItemSpacing = {12, 12};
    style.ItemInnerSpacing = {8, 6};
    style.Colors[ImGuiCol_WindowBg] = {0.055f, 0.063f, 0.086f, 1};
    style.Colors[ImGuiCol_ChildBg] = {0.085f, 0.098f, 0.13f, 1};
    style.Colors[ImGuiCol_Border] = {0.17f, 0.19f, 0.24f, 1};
    style.Colors[ImGuiCol_FrameBg] = {0.055f, 0.065f, 0.09f, 1};
    style.Colors[ImGuiCol_FrameBgHovered] = {0.13f, 0.15f, 0.22f, 1};
    style.Colors[ImGuiCol_FrameBgActive] = {0.17f, 0.19f, 0.28f, 1};
    style.Colors[ImGuiCol_Text] = {0.92f, 0.93f, 0.97f, 1};
    style.Colors[ImGuiCol_TextDisabled] = {0.55f, 0.60f, 0.69f, 1};
    style.Colors[ImGuiCol_Button] = {0.17f, 0.19f, 0.26f, 1};
    style.Colors[ImGuiCol_ButtonHovered] = {0.24f, 0.27f, 0.36f, 1};
    style.Colors[ImGuiCol_ButtonActive] = {0.31f, 0.30f, 0.48f, 1};
    style.Colors[ImGuiCol_Header] = {0.28f, 0.23f, 0.46f, 1};
    style.Colors[ImGuiCol_HeaderHovered] = {0.38f, 0.31f, 0.61f, 1};
    style.Colors[ImGuiCol_CheckMark] = {0.68f, 0.55f, 1, 1};
    style.Colors[ImGuiCol_NavCursor] = {0.68f, 0.55f, 1, 1};
    style.ScaleAllSizes(scale);
    if (!ImGui_ImplWin32_Init(window) || !ImGui_ImplDX11_Init(device.Get(), context.Get()))
    {
        throw hresult_error(E_FAIL);
    }
    initialized = true;
    BOOL dark = TRUE;
    DwmSetWindowAttribute(window, 20, &dark, sizeof(dark));
}

void InterfaceRenderer::resize(UINT width, UINT height)
{
    if (!swap || !width || !height)
    {
        return;
    }
    context->OMSetRenderTargets(0, nullptr, nullptr);
    target.Reset();
    check(swap->ResizeBuffers(0, width, height, DXGI_FORMAT_UNKNOWN, 0));
    createTarget();
}

void InterfaceRenderer::begin()
{
    ImGui_ImplDX11_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();
}

void InterfaceRenderer::end()
{
    ImGui::Render();
    float clear[] = {0.055f, 0.063f, 0.086f, 1};
    context->OMSetRenderTargets(1, target.GetAddressOf(), nullptr);
    context->ClearRenderTargetView(target.Get(), clear);
    ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
    check(swap->Present(0, 0));
}

void InterfaceRenderer::shutdown()
{
    if (initialized)
    {
        ImGui_ImplDX11_Shutdown();
        ImGui_ImplWin32_Shutdown();
        ImGui::DestroyContext();
        initialized = false;
    }
    target.Reset();
    swap.Reset();
    context.Reset();
    device.Reset();
}
} // namespace streamdiscord
