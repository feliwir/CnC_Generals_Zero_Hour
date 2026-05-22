#include "qsdlwindow.h"
#include <SDL3/SDL.h>
#include <qpa/qplatformnativeinterface.h>
#include <QGuiApplication>

void QSdlWindow::resizeEvent(QResizeEvent *event)
{
    QWindow::resizeEvent(event);
    qreal dpr = devicePixelRatio();
    QRect rect = geometry();
    SDL_SetWindowPosition(m_pSDLWindow, rect.x() * dpr, rect.y() * dpr);
    SDL_SetWindowSize(m_pSDLWindow, rect.width() * dpr, rect.height() * dpr);
}

void QSdlWindow::Initialize()
{
    if (m_pSDLWindow != NULL)
    {
        qDebug() << "SDL window already initialized.";
        return;
    }

    setFlag(Qt::FramelessWindowHint);
    setSurfaceType(QWindow::VulkanSurface);
    setGeometry(0, 0, 640, 480);
    m_pWindowId = reinterpret_cast<void *>(winId());

    QPlatformNativeInterface *native = QGuiApplication::platformNativeInterface();
    QString platform = QGuiApplication::platformName().toLower();

    if (!(SDL_WasInit(SDL_INIT_VIDEO) & SDL_INIT_VIDEO))
    {
        if (platform == "wayland")
        {
            struct wl_display *display = (struct wl_display *)native->nativeResourceForIntegration("display");
            SDL_SetPointerProperty(SDL_GetGlobalProperties(), SDL_PROP_GLOBAL_VIDEO_WAYLAND_WL_DISPLAY_POINTER, display);
        }

        if (!SDL_InitSubSystem(SDL_INIT_VIDEO | SDL_INIT_EVENTS))
        {
            qWarning() << "SDL video init failed:" << SDL_GetError();
            return;
        }
    }

    SDL_PropertiesID props = SDL_CreateProperties();
    if (!props)
    {
        qWarning() << "SDL_CreateProperties failed:" << SDL_GetError();
        return;
    }

    SDL_SetBooleanProperty(props, SDL_PROP_WINDOW_CREATE_VULKAN_BOOLEAN, true);
    SDL_SetNumberProperty(props, SDL_PROP_WINDOW_CREATE_WIDTH_NUMBER, 640);
    SDL_SetNumberProperty(props, SDL_PROP_WINDOW_CREATE_HEIGHT_NUMBER, 480);

#ifdef __linux__
    if (platform == "wayland")
    {
        qDebug() << "Setting SDL properties for Wayland.";
        struct wl_surface *surface = (struct wl_surface *)native->nativeResourceForWindow("surface", this);
        // Tell SDL to use Qt's Wayland display and wrap the existing Qt surface.
        SDL_SetPointerProperty(props, SDL_PROP_WINDOW_CREATE_WAYLAND_WL_SURFACE_POINTER, surface);
    }
    else if (platform == "xcb")
    {
        qDebug() << "Setting SDL properties for XCB.";
        SDL_SetNumberProperty(props, SDL_PROP_WINDOW_CREATE_X11_WINDOW_NUMBER, (Sint64)(uintptr_t)m_pWindowId);
    }
#endif
#ifdef _WIN32
    if (platform == "windows")
    {
        qDebug() << "Setting SDL properties for Windows.";
        SDL_SetPointerProperty(props, SDL_PROP_WINDOW_CREATE_WIN32_HWND_POINTER, m_pWindowId);
    }
#endif
#ifdef __APPLE__
    if (platform == "cocoa")
    {
        qDebug() << "Setting SDL properties for Cocoa.";
        SDL_SetPointerProperty(props, SDL_PROP_WINDOW_CREATE_COCOA_WINDOW_POINTER, m_pWindowId);
    }
#endif
    else
    {
        qWarning() << "Unknown Qt platform. SDL window may not integrate properly.";
    }

    m_pSDLWindow = SDL_CreateWindowWithProperties(props);
    SDL_DestroyProperties(props);

    requestUpdate();
}