#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#include <SDL.h>

#include <Net/HTTPStream.h>

#include "IO/PrivateStream.h"
#include "IO/SystemStream.h"
#include "Frontend/Context.h"
#include "Graphics/GLHeader.h"
#include "Binding/JSWindow.h"
#include "SDL_keycode_translate.h"

#define kNativeTitleBarHeight 0

uint32_t ttfps = 0;

namespace Nidium {
namespace Interface {

// {{{ NativUIInterface
NativeUIInterface::NativeUIInterface() :
    m_CurrentCursor(NativeUIInterface::ARROW), m_NativeCtx(NULL), m_Nml(NULL),
    m_Win(NULL), m_Gnet(NULL), m_Width(0), m_Height(0), m_FilePath(NULL),
    m_Initialized(false), m_IsOffscreen(false), m_ReadPixelInBuffer(false),
    m_Hidden(false), m_FBO(0), m_FrameBuffer(NULL), m_Console(NULL),
    m_MainGLCtx(NULL), m_SystemMenu(this)
{
    Nidium::Core::Path::RegisterScheme(SCHEME_DEFINE("file://",    Nidium::IO::FileStream,    false), true); // default
    Nidium::Core::Path::RegisterScheme(SCHEME_DEFINE("private://", Nidium::IO::PrivateStream, false));
#if 1
    Nidium::Core::Path::RegisterScheme(SCHEME_DEFINE("system://",  Nidium::IO::SystemStream,  false));
    Nidium::Core::Path::RegisterScheme(SCHEME_DEFINE("user://",    Nidium::IO::UserStream,    false));
#endif
    Nidium::Core::Path::RegisterScheme(SCHEME_DEFINE("http://",    Nidium::Net::HTTPStream,    true));
    Nidium::Core::Path::RegisterScheme(SCHEME_DEFINE("https://",   Nidium::Net::HTTPStream,    true));
    Nidium::Core::Path::RegisterScheme(SCHEME_DEFINE("nvfs://",    Nidium::IO::NFSStream,     false));

    Nidium::Core::TaskManager::CreateManager();
}

int NativeUIInterface::HandleEvents(NativeUIInterface *NUII)
{
    SDL_Event event;
    int nrefresh = 0;
    int nevents = 0;

    while(SDL_PollEvent(&event)) {
        Nidium::Binding::JSWindow *window = NULL;
        if (NUII->m_NativeCtx) {
            NUII->makeMainGLCurrent();
            window = Nidium::Binding::JSWindow::GetObject(NUII->m_NativeCtx->getNJS());
        }
        nevents++;
        switch(event.type) {
            case SDL_WINDOWEVENT:
                if (window) {
                    switch (event.window.event) {
                        case SDL_WINDOWEVENT_FOCUS_GAINED:
                            window->windowFocus();
                            break;
                        case SDL_WINDOWEVENT_FOCUS_LOST:
                            window->windowBlur();
                            break;
                        default:
                            break;
                    }
                }
                break;
            case SDL_TEXTINPUT:
                if (window && event.text.text && strlen(event.text.text) > 0) {
                    window->textInput(event.text.text);
                }
                break;
            case SDL_USEREVENT:
                break;
            case SDL_QUIT:
                if (window && !window->onClose()) {
                    break;
                }
                NUII->stopApplication();
                SDL_Quit();
                NUII->quitApplication();

                break;
            case SDL_MOUSEMOTION:
                if (window) {
                    window->mouseMove(event.motion.x, event.motion.y - kNativeTitleBarHeight,
                               event.motion.xrel, event.motion.yrel);
                }
                break;
            case SDL_MOUSEWHEEL:
            {
                int cx, cy;
                SDL_GetMouseState(&cx, &cy);
                if (window) {
                    window->mouseWheel(event.wheel.x, event.wheel.y, cx, cy - kNativeTitleBarHeight);
                }
                break;
            }
            case SDL_MOUSEBUTTONUP:
            case SDL_MOUSEBUTTONDOWN:
                if (window) {
                    window->mouseClick(event.button.x, event.button.y - kNativeTitleBarHeight,
                                event.button.state, event.button.button, event.button.clicks);
                }
            break;
            case SDL_KEYDOWN:
            case SDL_KEYUP:
            {
                int keyCode = 0;
                int mod = 0;

                if ((&event.key)->keysym.sym == SDLK_r &&
                    (event.key.keysym.mod & KMOD_CTRL) && event.type == SDL_KEYDOWN) {

                    if (++nrefresh > 1) {
                        break;
                    }

                    NUII->hitRefresh();

                    break;
                }
                if (event.key.keysym.sym >= 97 && event.key.keysym.sym <= 122) {
                    keyCode = event.key.keysym.sym - 32;
                } else {
                    keyCode = SDL_KEYCODE_TO_DOMCODE(event.key.keysym.sym);
                }

                if (event.key.keysym.mod & KMOD_SHIFT || SDL_KEYCODE_GET_CODE(keyCode) == 16) {
                    mod |= Nidium::Binding::NIDIUM_KEY_SHIFT;
                }
                if (event.key.keysym.mod & KMOD_ALT || SDL_KEYCODE_GET_CODE(keyCode) == 18) {
                    mod |= Nidium::Binding::NIDIUM_KEY_ALT;
                }
                if (event.key.keysym.mod & KMOD_CTRL || SDL_KEYCODE_GET_CODE(keyCode) == 17) {
                    mod |= Nidium::Binding::NIDIUM_KEY_CTRL;
                }
                if (event.key.keysym.mod & KMOD_GUI || SDL_KEYCODE_GET_CODE(keyCode) == 91) {
                    mod |= Nidium::Binding::NIDIUM_KEY_META;
                }
                if (window) {
                    window->keyupdown(SDL_KEYCODE_GET_CODE(keyCode), mod,
                        event.key.state, event.key.repeat,
                        SDL_KEYCODE_GET_LOCATION(keyCode));
                }

                break;
            }
        }
    }

    if (ttfps%300 == 0 && NUII->m_NativeCtx != NULL) {
        NUII->m_NativeCtx->getNJS()->gc();
    }

    if (NUII->m_CurrentCursor != NativeUIInterface::NOCHANGE) {
        NUII->setSystemCursor(NUII->m_CurrentCursor);
        NUII->m_CurrentCursor = NativeUIInterface::NOCHANGE;
    }

    if (NUII->m_NativeCtx) {
        NUII->makeMainGLCurrent();
        NUII->m_NativeCtx->frame(true);
    }
#if 0
    TODO : OSX
    if (NUII->getConsole()) {
        NUII->getConsole()->flush();
    }
#endif
    if (NUII->getFBO() != 0 && NUII->m_NativeCtx) {

        glReadBuffer(GL_COLOR_ATTACHMENT0);

        glReadPixels(0, 0, NUII->getWidth(), NUII->getHeight(), GL_RGBA, GL_UNSIGNED_BYTE, NUII->getFrameBufferData());
        uint8_t *pdata = NUII->getFrameBufferData();

        NUII->m_NativeCtx->rendered(pdata, NUII->getWidth(), NUII->getHeight());
    } else {
        NUII->makeMainGLCurrent();
        SDL_GL_SwapWindow(NUII->m_Win);
    }

    ttfps++;
    return 16;
}

bool NativeUIInterface::makeMainGLCurrent()
{
    if (!m_MainGLCtx) return false;
    return (SDL_GL_MakeCurrent(this->m_Win, m_MainGLCtx) == 0);
}

SDL_GLContext NativeUIInterface::getCurrentGLContext()
{
    return SDL_GL_GetCurrentContext();
}

bool NativeUIInterface::makeGLCurrent(SDL_GLContext ctx)
{
    return (SDL_GL_MakeCurrent(this->m_Win, ctx) == 0);
}

SDL_GLContext NativeUIInterface::createSharedContext(bool webgl)
{
    SDL_GL_SetAttribute(SDL_GL_SHARE_WITH_CURRENT_CONTEXT, 1);

    SDL_GLContext created = SDL_GL_CreateContext(this->m_Win);

    return created;
}

void NativeUIInterface::setWindowTitle(const char *name)
{
    SDL_SetWindowTitle(m_Win, (name == NULL || *name == '\0' ? "nidium" : name));
}

const char *NativeUIInterface::getWindowTitle() const
{
    return SDL_GetWindowTitle(m_Win);
}


void NativeUIInterface::setCursor(CURSOR_TYPE type)
{
    this->m_CurrentCursor = type;
}


void NativeUIInterface::deleteGLContext(SDL_GLContext ctx)
{
    SDL_GL_DeleteContext(ctx);
}

void NativeUIInterface::quit()
{
    this->stopApplication();
    SDL_Quit();
}

void NativeUIInterface::refresh()
{
    int oswap = SDL_GL_GetSwapInterval();
    SDL_GL_SetSwapInterval(0);

    if (this->m_NativeCtx) {
        this->makeMainGLCurrent();
        this->m_NativeCtx->frame();
    }

    SDL_GL_SwapWindow(this->m_Win);

    SDL_GL_SetSwapInterval(oswap);
}

void NativeUIInterface::centerWindow()
{
    SDL_SetWindowPosition(this->m_Win, SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED);
}

void NativeUIInterface::getScreenSize(int *width, int *height)
{
    SDL_Rect bounds;
    int displayIndex = SDL_GetWindowDisplayIndex(this->m_Win);

    SDL_GetDisplayBounds(displayIndex, &bounds);

    if (width) *width = bounds.w;
    if (height) *height = bounds.h;
}

void NativeUIInterface::setWindowPosition(int x, int y)
{
    SDL_SetWindowPosition(this->m_Win,
        (x == NATIVE_WINDOWPOS_UNDEFINED_MASK) ? SDL_WINDOWPOS_UNDEFINED_MASK : x,
        (y == NATIVE_WINDOWPOS_UNDEFINED_MASK) ? SDL_WINDOWPOS_UNDEFINED_MASK : y);
}

void NativeUIInterface::getWindowPosition(int *x, int *y)
{
    SDL_GetWindowPosition(this->m_Win, x, y);
}

void NativeUIInterface::setWindowSize(int w, int h)
{
    this->m_Width = w;
    this->m_Height = h;

    SDL_SetWindowSize(this->m_Win, w, h);
}

void NativeUIInterface::setWindowFrame(int x, int y, int w, int h)
{
    if (x == NATIVE_WINDOWPOS_CENTER_MASK) x = SDL_WINDOWPOS_CENTERED;
    if (y == NATIVE_WINDOWPOS_CENTER_MASK) y = SDL_WINDOWPOS_CENTERED;

    this->setWindowSize(w, h);
    this->setWindowPosition(x, y);
}

void NativeUIInterface::toggleOfflineBuffer(bool val)
{
    if (val && !m_ReadPixelInBuffer) {
        this->initPBOs();
    } else if (!val && m_ReadPixelInBuffer) {

        glDeleteBuffers(NUM_PBOS, m_PBOs.pbo);
        free(m_FrameBuffer);
    }
    m_ReadPixelInBuffer = val;
}

void NativeUIInterface::initPBOs()
{
    if (m_ReadPixelInBuffer) {
        return;
    }

    uint32_t screenPixelSize = m_Width * 2 * m_Height * 2 * 4;

    glGenBuffers(NUM_PBOS, m_PBOs.pbo);
    for (int i = 0; i < NUM_PBOS; i++) {
        glBindBuffer(GL_PIXEL_PACK_BUFFER, m_PBOs.pbo[i]);
        glBufferData(GL_PIXEL_PACK_BUFFER, screenPixelSize, 0, GL_DYNAMIC_READ);
    }

    glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);

    m_PBOs.vram2sys = 0;
    m_PBOs.gpu2vram = NUM_PBOS-1;

    m_FrameBuffer = (uint8_t *)malloc(screenPixelSize);
}

uint8_t *NativeUIInterface::readScreenPixel()
{
    if (!m_ReadPixelInBuffer) {
        this->toggleOfflineBuffer(true);
    }

    //uint32_t screenPixelSize = m_Width * 2 * m_Height * 2 * 4;

    glReadBuffer(GL_COLOR_ATTACHMENT0);

    glBindBuffer(GL_PIXEL_PACK_BUFFER, m_PBOs.pbo[m_PBOs.gpu2vram]);

    glReadPixels(0, 0, m_Width*2, m_Height*2, GL_RGBA, GL_UNSIGNED_BYTE, 0);

    glBindBuffer(GL_PIXEL_PACK_BUFFER, m_PBOs.pbo[m_PBOs.vram2sys]);
    uint8_t *ret = (uint8_t *)glMapBuffer(GL_PIXEL_PACK_BUFFER, GL_READ_ONLY);
    if (!ret) {
        uint32_t err = glGetError();
        fprintf(stderr, "Failed to map buffer: Error %d\n", err);
        return NULL;
    }

    /* Flip Y pixels (row by row) */
    for (uint32_t i = 0; i < m_Height * 2; i++) {
        memcpy(m_FrameBuffer + i * m_Width * 2 * 4,
            &ret[(m_Height*2 - i - 1) * m_Width * 2 * 4], m_Width*2*4);
    }

    //memcpy(m_FrameBuffer, ret, screenPixelSize);

    glUnmapBuffer(GL_PIXEL_PACK_BUFFER);
    glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);

    int temp = m_PBOs.pbo[0];
    for (int i=1; i<NUM_PBOS; i++)
        m_PBOs.pbo[i-1] = m_PBOs.pbo[i];
    m_PBOs.pbo[NUM_PBOS - 1] = temp;

    return m_FrameBuffer;
}

int NativeUIInterface::useOffScreenRendering(bool val)
{
    if (!val && m_IsOffscreen) {
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        m_IsOffscreen = false;
        m_FBO = 0;
        free(m_FrameBuffer);
        m_FrameBuffer = NULL;
        // TODO : delete fbo & renderbuffer
        return 0;
    }

    if (val && !m_IsOffscreen) {
        GLuint fbo, render_buf;
        glGenFramebuffers(1, &fbo);
        glGenRenderbuffers(1, &render_buf);

        glBindRenderbuffer(GL_RENDERBUFFER, render_buf);
        glRenderbufferStorage(GL_RENDERBUFFER, GL_RGBA4, m_Width, m_Height);

        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, fbo);
        glFramebufferRenderbuffer(GL_DRAW_FRAMEBUFFER,
            GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, render_buf);

        m_FBO = fbo;

        m_FrameBuffer = (uint8_t *)malloc(m_Width*m_Height*4);

        SDL_HideWindow(m_Win);

        return fbo;
    }

    return 0;
}

void NativeUIInterface::refreshApplication(bool clearConsole)
{

    if (clearConsole) {
        NativeUIConsole *console = this->getConsole();
        if (console && !console->hidden()) {
            console->clear();
        }
    }

    /* Trigger GC before refreshing */
    if (m_NativeCtx && m_NativeCtx->getNJS()) {
        m_NativeCtx->getNJS()->gc();
    }

    this->restartApplication();
}

void NativeUIInterface::hideWindow()
{
    if (!m_Hidden) {
        m_Hidden = true;
        SDL_HideWindow(m_Win);

        APE_timer_setlowresolution(this->m_Gnet, 1);
    }
}

void NativeUIInterface::showWindow()
{
    if (m_Hidden) {
        m_Hidden = false;
        SDL_ShowWindow(m_Win);

        APE_timer_setlowresolution(this->m_Gnet, 0);
    }
}
// }}}

// {{{ NativeSystemMenu
void NativeSystemMenu::addItem(NativeSystemMenuItem *item)
{
    item->m_Next = m_Items;
    m_Items = item;
}

void NativeSystemMenu::deleteItems()
{
    NativeSystemMenuItem *tmp = NULL, *cur = m_Items;
    while (cur != NULL) {
        tmp = cur->m_Next;
        delete cur;
        cur = tmp;
    }

    m_Items = NULL;
}

void NativeSystemMenu::setIcon(const uint8_t *data, size_t width, size_t height)
{
    m_Icon.data = data;
    m_Icon.len = width * height * 4;
    m_Icon.width = width;
    m_Icon.height = height;
}

NativeSystemMenu::NativeSystemMenu(NativeUIInterface *ui) : m_UI(ui)
{
    m_Items = NULL;
    m_Icon.data = NULL;
    m_Icon.len = 0;
}

NativeSystemMenu::~NativeSystemMenu()
{
    this->deleteItems();
}
// }}}

} // namespace Nidium
} // namespace Interface

