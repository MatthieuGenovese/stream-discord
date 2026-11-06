#include "application.h"
#include <winrt/base.h>
int WINAPI wWinMain(HINSTANCE instance, HINSTANCE, PWSTR, int)
{
    winrt::init_apartment(winrt::apartment_type::single_threaded);
    SetProcessDPIAware();
    streamdiscord::Application application;
    return application.run(instance);
}
