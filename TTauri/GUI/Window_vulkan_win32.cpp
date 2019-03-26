
#include "Window_vulkan_win32.hpp"

#include "Instance_vulkan.hpp"

#include "TTauri/Application_win32.hpp"
#include "TTauri/strings.hpp"

namespace TTauri {
namespace GUI {

using namespace std;
using namespace TTauri;

inline gsl::not_null<void *>to_ptr(LPARAM lParam)
{
    void *ptr;
    memcpy(&ptr, &lParam, sizeof(void *));

    return gsl::not_null<void *>(ptr);
}

const wchar_t *Window_vulkan_win32::win32WindowClassName = nullptr;
WNDCLASS Window_vulkan_win32::win32WindowClass = {};
bool Window_vulkan_win32::win32WindowClassIsRegistered = false;
std::shared_ptr<std::unordered_map<HWND, Window_vulkan_win32 *>> Window_vulkan_win32::win32WindowMap = {};
bool Window_vulkan_win32::firstWindowHasBeenOpened = false;

void Window_vulkan_win32::createWindowClass()
{
    if (!Window_vulkan_win32::win32WindowClassIsRegistered) {
         // Register the window class.
        Window_vulkan_win32::win32WindowClassName = L"TTauri Window Class";

        Window_vulkan_win32::win32WindowClass.lpfnWndProc = Window_vulkan_win32::_WindowProc;
        Window_vulkan_win32::win32WindowClass.hInstance = get_singleton<Application_win32>()->hInstance;
        Window_vulkan_win32::win32WindowClass.lpszClassName = Window_vulkan_win32::win32WindowClassName;
        Window_vulkan_win32::win32WindowClass.hCursor = LoadCursor(nullptr, IDC_ARROW);
        RegisterClass(&win32WindowClass);
    }
    Window_vulkan_win32::win32WindowClassIsRegistered = true;
}

vk::SurfaceKHR Window_vulkan_win32::createWindow(const std::string &title)
{
    Window_vulkan_win32::createWindowClass();

    auto u16title = translateString<wstring>(title);

    win32Window = CreateWindowExW(
        0, // Optional window styles.
        Window_vulkan_win32::win32WindowClassName, // Window class
        u16title.data(), // Window text
        WS_OVERLAPPEDWINDOW, // Window style

        // Size and position
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        CW_USEDEFAULT,

        NULL, // Parent window
        NULL, // Menu
        get_singleton<Application_win32>()->hInstance, // Instance handle
        this
    );

    if (win32Window == nullptr) {
        BOOST_THROW_EXCEPTION(Application::Error());
    }

    if (!Window_vulkan_win32::firstWindowHasBeenOpened) {
        ShowWindow(win32Window, get_singleton<Application_win32>()->nCmdShow);
        Window_vulkan_win32::firstWindowHasBeenOpened = true;
    }
    ShowWindow(win32Window, SW_SHOW);

    return get_singleton<Instance_vulkan>()->intrinsic.createWin32SurfaceKHR({
        vk::Win32SurfaceCreateFlagsKHR(),
        get_singleton<Application_win32>()->hInstance,
        win32Window
    });
}

Window_vulkan_win32::Window_vulkan_win32(const std::shared_ptr<Window::Delegate> &delegate, const std::string &title) :
    Window_vulkan(delegate, title, createWindow(title))
{
}

Window_vulkan_win32::~Window_vulkan_win32()
{
    try {
        [[gsl::suppress(f.6)]] {
            if (win32Window != nullptr) {
                LOG_FATAL("win32Window was not destroyed before Window '%s' was destructed.") % title;
                abort();
            }
        }
    } catch (...) {
        abort();
    }
}


LRESULT Window_vulkan_win32::windowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg) {
    case WM_MOVING: {
        RECT windowRect;
        memcpy(&windowRect, to_ptr(lParam).get(), sizeof (RECT));

        setWindowPosition(windowRect.left, windowRect.top);
        break;
    }

    case WM_SIZING: {
        RECT windowRect;
        memcpy(&windowRect, to_ptr(lParam).get(), sizeof (RECT));

        setWindowSize(windowRect.right - windowRect.left, windowRect.bottom - windowRect.top);
        break;
    }

    case WM_DESTROY:
        win32Window = nullptr;
        break;

    }
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

LRESULT CALLBACK Window_vulkan_win32::_WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    if (!win32WindowMap) {
        win32WindowMap = make_shared<std::unordered_map<HWND, Window_vulkan_win32 *>>();
    }

    if (uMsg == WM_NCCREATE && lParam) {
        [[gsl::suppress(type.1)]] {
            auto const createData = reinterpret_cast<CREATESTRUCT *>(lParam);

            if (createData->lpCreateParams) {
                [[gsl::suppress(lifetime.1)]] {
                    (*Window_vulkan_win32::win32WindowMap)[hwnd] = static_cast<Window_vulkan_win32 *>(createData->lpCreateParams);
                }
            }
        }
    }

    auto i = Window_vulkan_win32::win32WindowMap->find(hwnd);
    if (i != Window_vulkan_win32::win32WindowMap->end()) {
        auto const window = i->second;
        auto const result = window->windowProc(hwnd, uMsg, wParam, lParam);

        if (uMsg == WM_DESTROY) {
            Window_vulkan_win32::win32WindowMap->erase(i);
        }

        return result;
    }

    // Fallback.
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

}}