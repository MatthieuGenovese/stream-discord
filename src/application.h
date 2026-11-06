#pragma once

#include "audio_capture.h"
#include "audio_devices.h"
#include "interface.h"
#include "video_capture.h"
#include "window_sources.h"

#include <string>
#include <vector>

namespace streamdiscord
{
class Application
{
public:
    int run(HINSTANCE instance);

private:
    static LRESULT CALLBACK windowProcedure(HWND window, UINT message, WPARAM wParam,
                                            LPARAM lParam);
    static LRESULT CALLBACK diffusionProcedure(HWND window, UINT message, WPARAM wParam,
                                               LPARAM lParam);
    LRESULT handleMessage(HWND window, UINT message, WPARAM wParam, LPARAM lParam);

    void refreshOutputs();
    void save();
    void stop();
    void showDiffusion();
    void reconcile();
    void startCapture();
    void initializeWindows(HWND window);
    void updateFrame(HWND window);

    void drawInterface();
    void drawHeader();
    void drawVideoCard(float width, float scale);
    void drawAudioCard(float width, float scale);
    void drawOutputCard(float scale);
    void drawStatusCard(float scale);
    void drawActions(float scale);
    void drawApplicationPicker(const char* id, char* buffer, size_t size);

    HWND videoWindow = nullptr;
    VideoCapture video;
    AudioCapture audio;
    InterfaceRenderer interfaceRenderer;

    bool armed = false;
    bool uiReady = false;
    HWND captured = nullptr;
    DWORD audioPid = 0;

    std::wstring videoName;
    std::wstring audioName;
    std::wstring iniPath;
    std::wstring videoError;
    std::wstring outputId;
    std::wstring activeOutputId;
    std::wstring statusText = L"Sources prêtes à configurer";
    std::string uiError;

    char videoInput[512]{};
    char audioInput[512]{};
    std::vector<WindowSource> choices;
    std::vector<OutputDevice> outputs;
};
} // namespace streamdiscord
