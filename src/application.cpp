#include "application.h"
#include "backends/imgui_impl_win32.h"
#include "common.h"
#include "imgui.h"
#include "settings.h"
#include "text.h"
#include <algorithm>
#include <cstdio>
#include <dwmapi.h>

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND, UINT, WPARAM, LPARAM);
namespace streamdiscord
{
using namespace winrt;

namespace
{
constexpr UINT_PTR SourceTimer = 1;
constexpr UINT_PTR FrameTimer = 2;
constexpr UINT SourcePollIntervalMs = 750;
constexpr UINT FrameIntervalMs = 16;
} // namespace

void Application::refreshOutputs()
{
    outputs = enumerateAudioOutputs();
    const bool selectedDeviceExists = std::any_of(outputs.begin(), outputs.end(),
                                                  [this](const auto& output)
                                                  {
                                                      return output.id == outputId;
                                                  });
    if (!selectedDeviceExists)
    {
        outputId.clear();
    }
}

void Application::save()
{
    saveSettings(iniPath, {toWide(videoInput), toWide(audioInput), outputId});
}

void Application::startCapture()
{
    videoName = toWide(videoInput);
    audioName = toWide(audioInput);
    if (videoName.empty())
    {
        uiError = "Une source vidéo est requise.";
        return;
    }
    save();
    armed = true;
    showDiffusion();
    reconcile();
}

void Application::stop()
{
    video.stop();
    audio.stop();
    captured = nullptr;
    audioPid = 0;
    if (IsWindow(videoWindow))
    {
        InvalidateRect(videoWindow, nullptr, TRUE);
    }
}

void Application::showDiffusion()
{
    ShowWindow(videoWindow, IsIconic(videoWindow) ? SW_RESTORE : SW_SHOW);
    SetForegroundWindow(videoWindow);
}

void Application::reconcile()
{
    auto availableWindows = enumerateWindows();
    auto videoSource = std::find_if(availableWindows.begin(), availableWindows.end(),
                                    [this](auto const& source)
                                    {
                                        return lowercase(source.exe) == lowercase(videoName);
                                    });
    if (videoSource == availableWindows.end())
    {
        if (captured)
        {
            stop();
        }
        statusText = L"En attente de " + videoName;
        return;
    }
    if (captured != videoSource->hwnd)
    {
        stop();
        try
        {
            video.start(videoSource->hwnd, videoWindow);
            captured = videoSource->hwnd;
            videoError.clear();
        }
        catch (hresult_error const& error)
        {
            videoError = error.message().c_str();
            video.stop();
        }
    }
    DWORD requestedAudioPid = videoSource->pid;
    if (!audioName.empty())
    {
        requestedAudioPid = 0;
        auto audioSource = std::find_if(availableWindows.begin(), availableWindows.end(),
                                        [this](auto const& source)
                                        {
                                            return lowercase(source.exe) == lowercase(audioName);
                                        });
        if (audioSource != availableWindows.end())
        {
            requestedAudioPid = audioSource->pid;
        }
    }
    if (requestedAudioPid != audioPid || activeOutputId != outputId)
    {
        audio.stop();
        audioPid = requestedAudioPid;
        activeOutputId = outputId;
        if (requestedAudioPid && !outputId.empty())
        {
            audio.start(requestedAudioPid, outputId);
        }
    }
    statusText =
        captured ? L"Vidéo : " + videoSource->exe : L"Capture indisponible : " + videoError;
    HRESULT audioStatus = audio.currentStatus();
    if (outputId.empty())
    {
        statusText += L"  ·  Relais audio désactivé";
    }
    else if (!requestedAudioPid)
    {
        statusText += L"  ·  En attente de la source audio";
    }
    else if (audioStatus == E_PENDING)
    {
        statusText += L"  ·  Connexion audio";
    }
    else if (FAILED(audioStatus))
    {
        wchar_t code[32];
        swprintf_s(code, L"0x%08X", static_cast<unsigned>(audioStatus));
        statusText += L"  ·  Erreur audio " + std::wstring(code);
    }
    else
    {
        statusText += L"  ·  Audio : " + (audioName.empty() ? videoSource->exe : audioName);
    }
}

LRESULT CALLBACK Application::diffusionProcedure(HWND window, UINT message, WPARAM wParam,
                                                 LPARAM lParam)
{
    if (message == WM_CLOSE)
    {
        ShowWindow(window, SW_HIDE);
        return 0;
    }
    return DefWindowProcW(window, message, wParam, lParam);
}

LRESULT CALLBACK Application::windowProcedure(HWND window, UINT message, WPARAM wParam,
                                              LPARAM lParam)
{
    auto* application = reinterpret_cast<Application*>(GetWindowLongPtrW(window, GWLP_USERDATA));
    if (message == WM_NCCREATE)
    {
        const auto* create = reinterpret_cast<const CREATESTRUCTW*>(lParam);
        application = static_cast<Application*>(create->lpCreateParams);
        SetWindowLongPtrW(window, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(application));
    }
    if (application)
    {
        return application->handleMessage(window, message, wParam, lParam);
    }
    return DefWindowProcW(window, message, wParam, lParam);
}

void Application::initializeWindows(HWND window)
{
    WNDCLASSW windowClass{};
    windowClass.lpfnWndProc = diffusionProcedure;
    windowClass.hInstance = GetModuleHandleW(nullptr);
    windowClass.lpszClassName = L"StreamDiscordDiffusion";
    windowClass.hCursor = LoadCursor(nullptr, IDC_ARROW);
    windowClass.hbrBackground = reinterpret_cast<HBRUSH>(GetStockObject(BLACK_BRUSH));
    RegisterClassW(&windowClass);
    videoWindow = CreateWindowExW(0, windowClass.lpszClassName, L"StreamDiscord — Diffusion",
                                  WS_OVERLAPPEDWINDOW, 160, 400, 960, 540, nullptr, nullptr,
                                  windowClass.hInstance, nullptr);
    if (!videoWindow)
    {
        throw hresult_error(HRESULT_FROM_WIN32(GetLastError()));
    }
    BOOL dark = TRUE;
    DwmSetWindowAttribute(videoWindow, 20, &dark, sizeof(dark));
    interfaceRenderer.init(window);
    uiReady = true;
    choices = enumerateWindows();
    refreshOutputs();
    SetTimer(window, SourceTimer, SourcePollIntervalMs, nullptr);
    SetTimer(window, FrameTimer, FrameIntervalMs, nullptr);
}

void Application::updateFrame(HWND window)
{
    if (armed && captured)
    {
        try
        {
            video.tick();
        }
        catch (hresult_error const& error)
        {
            videoError = error.message().c_str();
            stop();
            statusText = L"Capture interrompue : " + videoError;
        }
    }
    if (!IsIconic(window))
    {
        interfaceRenderer.begin();
        drawInterface();
        interfaceRenderer.end();
    }
}

LRESULT Application::handleMessage(HWND window, UINT message, WPARAM wParam, LPARAM lParam)
{
    if (uiReady && ImGui_ImplWin32_WndProcHandler(window, message, wParam, lParam))
    {
        return TRUE;
    }
    try
    {
        switch (message)
        {
        case WM_CREATE:
            initializeWindows(window);
            return 0;
        case WM_GETMINMAXINFO:
        {
            auto info = reinterpret_cast<MINMAXINFO*>(lParam);
            float scale = interfaceRenderer.dpiScale();
            info->ptMinTrackSize = {static_cast<LONG>(1000 * scale),
                                    static_cast<LONG>(800 * scale)};
            return 0;
        }
        case WM_SIZE:
            if (uiReady && wParam != SIZE_MINIMIZED)
            {
                interfaceRenderer.resize(LOWORD(lParam), HIWORD(lParam));
            }
            return 0;
        case WM_TIMER:
            if (wParam == SourceTimer && armed)
            {
                reconcile();
            }
            if (wParam == FrameTimer)
            {
                updateFrame(window);
            }
            return 0;
        case WM_DESTROY:
            KillTimer(window, SourceTimer);
            KillTimer(window, FrameTimer);
            stop();
            DestroyWindow(videoWindow);
            uiReady = false;
            interfaceRenderer.shutdown();
            PostQuitMessage(0);
            return 0;
        }
    }
    catch (hresult_error const& error)
    {
        uiError = toUtf8(error.message().c_str());
        if (message == WM_CREATE)
        {
            MessageBoxW(window, error.message().c_str(), L"Initialisation impossible",
                        MB_ICONERROR);
            return -1;
        }
    }
    return DefWindowProcW(window, message, wParam, lParam);
}

int Application::run(HINSTANCE instance)
{
    iniPath = settingsPath();
    const auto settings = loadSettings(iniPath);
    videoName = settings.videoExecutable;
    audioName = settings.audioExecutable;
    outputId = settings.outputDeviceId;
    copyUtf8(videoInput, sizeof(videoInput), videoName);
    copyUtf8(audioInput, sizeof(audioInput), audioName);
    WNDCLASSW windowClass{};
    windowClass.lpfnWndProc = windowProcedure;
    windowClass.hInstance = instance;
    windowClass.lpszClassName = L"StreamDiscordControl";
    windowClass.hCursor = LoadCursor(nullptr, IDC_ARROW);
    RegisterClassW(&windowClass);
    float scale = GetDpiForSystem() / 96.0f;
    HWND window =
        CreateWindowW(windowClass.lpszClassName, L"StreamDiscord", WS_OVERLAPPEDWINDOW | WS_VISIBLE,
                      CW_USEDEFAULT, CW_USEDEFAULT, static_cast<int>(1120 * scale),
                      static_cast<int>(850 * scale), nullptr, nullptr, instance, this);
    if (!window)
    {
        return 1;
    }
    MSG message;
    while (GetMessageW(&message, nullptr, 0, 0) > 0)
    {
        TranslateMessage(&message);
        DispatchMessageW(&message);
    }
    return 0;
}
} // namespace streamdiscord
