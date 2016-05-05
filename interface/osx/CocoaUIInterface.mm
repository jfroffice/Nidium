/*
   Copyright 2016 Nidium Inc. All rights reserved.
   Use of this source code is governed by a MIT license
   that can be found in the LICENSE file.
*/
#import "CocoaUIInterface.h"

#import <sys/stat.h>
#import <objc/runtime.h>

#import <Cocoa/Cocoa.h>

#import <native_netlib.h>

#import "CocoaUIInterface.h"
#import <SDL.h>
#import <SDL_opengl.h>
#import <SDL_syswm.h>
#import <Cocoa/Cocoa.h>
#import <sys/stat.h>

#import <Core/Messages.h>
#import <Core/Path.h>
#import <Binding/NidiumJS.h>

#import <Frontend/Context.h>
#import <Frontend/NML.h>
#import <Frontend/App.h>
#import <Binding/JSWindow.h>

#import "System.h"
#import "UIConsole.h"
#import "DragNSView.h"

#define kNativeTitleBarHeight 0

#define kNativeVSYNC 1

@interface UICocoaInterfaceWrapper: NSObject {
    UICocoaInterface *base;
}

- (NSMenu *) renderSystemTray;
- (void) menuClicked:(id)ev;
- (id) initWithUI:(UICocoaInterface *)ui;

@end

uint64_t ttfps = 0;

static __inline__ void ConvertNSRect(NSRect *r)
{
    r->origin.y = CGDisplayPixelsHigh(kCGDirectMainDisplay) - r->origin.y - r->size.height;
}

static NSWindow *NativeCocoaWindow(SDL_Window *win)
{
    SDL_SysWMinfo info;
    SDL_VERSION(&info.version);
    SDL_GetWindowWMInfo(win, &info);

    return (NSWindow *)info.info.cocoa.window;
}

void UICocoaInterface::quitApplication()
{
    [[NSApplication sharedApplication] terminate:nil];
}

static int NativeProcessSystemLoop(void *arg)
{
    SDL_PumpEvents();
    UICocoaInterface *ui = (UICocoaInterface *)arg;

    if (ui->m_NativeCtx) {
        ui->makeMainGLCurrent();
    }
    return 4;
}

static void NativeDoneExtracting(void *closure, const char *fpath)
{
    UICocoaInterface *ui = (UICocoaInterface *)closure;
    if (chdir(fpath) != 0) {
        fprintf(stderr, "Cant enter cache directory (%d)\n", errno);
        return;
    }
    fprintf(stdout, "Changing directory to : %s\n", fpath);

    ui->m_Nml = new Nidium::Frontend::NML(ui->m_Gnet);
    ui->m_Nml->loadFile("./index.nml", UICocoaInterface_onNMLLoaded, ui);
}

void UICocoaInterface::log(const char *buf)
{
    if (this->m_Console && !this->m_Console->m_IsHidden) {
        this->m_Console->log(buf);
    } else {
        fwrite(buf, sizeof(char), strlen(buf), stdout);
        fflush(stdout);
    }
}

void UICocoaInterface::logf(const char *format, ...)
{
    char *buff;
    int len;
    va_list val;

    va_start(val, format);
    len = vasprintf(&buff, format, val);
    va_end(val);

    this->log(buff);

    free(buff);
}

void UICocoaInterface::vlog(const char *format, va_list ap)
{
    char *buff;
    int len;

    len = vasprintf(&buff, format, ap);

    this->log(buff);

    free(buff);
}

void UICocoaInterface::logclear()
{
    if (this->m_Console && !this->m_Console->m_IsHidden) {
        this->m_Console->clear();
    }
}


void UICocoaInterface::stopApplication()
{
    [this->m_DragNSView setResponder:nil];
    this->disableSysTray();
    m_SystemMenu.deleteItems();

    UIInterface::stopApplication();
}

UICocoaInterface::UICocoaInterface() :
    NativeUIInterface(), m_StatusItem(NULL), m_DragNSView(nil)
{
    m_Wrapper = [[UICocoaInterfaceWrapper alloc] initWithUI:this];
}

NativeUICocoaConsole *UICocoaInterface::getConsole(bool create, bool *created) {
    if (created) *created = false;
    if (this->m_Console == NULL && create) {
        this->m_Console = new UICocoaConsole;
        if (created) *created = true;
    }
    return this->m_Console;
}

void UICocoaInterface::onWindowCreated()
{
    NSWindow *window = NativeCocoaWindow(m_Win);

    m_DragNSView = [[DragNSView alloc] initWithFrame:NSMakeRect(0, 0, width, height)];
    [[window contentView] addSubview:this->m_DragNSView];
    [this->m_DragNSView setResponder:NativeJSwindow::getNativeClass(m_NativeCtx->getNJS())];

    this->patchSDLView([window contentView]);

    [window setCollectionBehavior:
             NSWindowCollectionBehaviorFullScreenPrimary];

    [window setFrameAutosaveName:@"nativeMainWindow"];
    if (kNativeTitleBarHeight != 0) {
        [window setStyleMask:NSTexturedBackgroundWindowMask|NSTitledWindowMask];
    }

#if NIDIUM_ENABLE_HIDPI
    NSView *openglview = [window contentView];
    [openglview setWantsBestResolutionOpenGLSurface:YES];
#endif

}

void UICocoaInterface::setTitleBarRGBAColor(uint8_t r, uint8_t g,
    uint8_t b, uint8_t a)
{
    NSWindow *window = NativeCocoaWindow(m_Win);
    NSUInteger mask = [window styleMask];

    fprintf(stdout, "setting titlebar color\n");

    if ((mask & NSTexturedBackgroundWindowMask) == 0) {
        [window setStyleMask:mask|NSTexturedBackgroundWindowMask];
        [window setMovableByWindowBackground:NO];
        [window setOpaque:NO];
    }

    [window setBackgroundColor:[NSColor
        colorWithSRGBRed:((double)r)/255.
                   green:((double)g)/255.
                    blue:((double)b)/255.
                   alpha:((double)a)/255]];
}

void UICocoaInterface::initControls()
{
    NSWindow *window = NativeCocoaWindow(m_Win);
    NSButton *close = [window standardWindowButton:NSWindowCloseButton];
    NSButton *min = [window standardWindowButton:NSWindowMiniaturizeButton];
    NSButton *max = [window standardWindowButton:NSWindowZoomButton];

    memset(&this->controls, 0, sizeof(CGRect));

    if (close) {
        this->controls.closeFrame = close.frame;
    }
    if (min) {
        this->controls.minFrame = min.frame;
    }
    if (max) {
        this->controls.zoomFrame = max.frame;
    }
}

void UICocoaInterface::setWindowControlsOffset(double x, double y)
{
    NSWindow *window = NativeCocoaWindow(m_Win);
    NSButton *close = [window standardWindowButton:NSWindowCloseButton];
    NSButton *min = [window standardWindowButton:NSWindowMiniaturizeButton];
    NSButton *max = [window standardWindowButton:NSWindowZoomButton];

    if (close) {
        [close setFrame:CGRectMake(
            this->controls.closeFrame.origin.x+x,
            this->controls.closeFrame.origin.y-y,
            this->controls.closeFrame.size.width,
            this->controls.closeFrame.size.height)];
    }
    if (min) {
        [min setFrame:CGRectMake(
            this->controls.minFrame.origin.x+x,
            this->controls.minFrame.origin.y-y,
            this->controls.minFrame.size.width,
            this->controls.minFrame.size.height)];
    }
    if (max) {
        [max setFrame:CGRectMake(
            this->controls.zoomFrame.origin.x+x,
            this->controls.zoomFrame.origin.y-y,
            this->controls.zoomFrame.size.width,
            this->controls.zoomFrame.size.height)];
    }
}

void UICocoaInterface::openFileDialog(const char *files[],
    void (*cb)(void *nof, const char *lst[], uint32_t len), void *arg, int flags)
{
    NSOpenPanel* openDlg = [NSOpenPanel openPanel];

    if (files != NULL) {
        NSMutableArray *fileTypesArray = [NSMutableArray arrayWithCapacity:0];

        for (int i = 0; files[i] != NULL; i++) {
            [fileTypesArray addObject:[NSString stringWithCString:files[i] encoding:NSASCIIStringEncoding]];
        }
        [openDlg setAllowedFileTypes:fileTypesArray];
    }

    [openDlg setCanChooseFiles:(flags & kOpenFile_CanChooseFile ? YES : NO)];
    [openDlg setCanChooseDirectories:(flags & kOpenFile_CanChooseDir ? YES : NO)];
    [openDlg setAllowsMultipleSelection:(flags & kOpenFile_AlloMultipleSelection ? YES : NO)];

    //[openDlg runModal];

#if 1
    // TODO: set a flag so that nidium can't be refreshed and unset after the block is called

    [openDlg beginSheetModalForWindow:NativeCocoaWindow(m_Win) completionHandler:^(NSInteger result) {
        if (result == NSFileHandlingPanelOKButton) {
            uint32_t len = [openDlg.URLs count];
            const char **lst = (const char **)malloc(sizeof(char **) * len);

            int i = 0;
            for (NSURL *url in openDlg.URLs) {
                lst[i] = [[url relativePath] UTF8String];
                i++;
            }

            cb(arg, lst, i);

            free(lst);
        } else {
            cb(arg, NULL, 0);
        }
    }];

    this->makeMainGLCurrent();
#endif
}

void UICocoaInterface::runLoop()
{
    APE_timer_create(m_Gnet, 1, NativeProcessUI, (void *)this);
    APE_timer_create(m_Gnet, 1, NativeProcessSystemLoop, (void *)this);

    APE_loop_run(m_Gnet);
}

void UICocoaInterface::setWindowFrame(int x, int y, int w, int h)
{
    NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
    NSWindow *nswindow = NativeCocoaWindow(m_Win);

    this->m_Width = w;
    this->m_Height = h;
    CGRect oldframe = [nswindow frame];

    int screen_width, screen_height;
    if (x == NIDIUM_WINDOWPOS_CENTER_MASK || y == NIDIUM_WINDOWPOS_CENTER_MASK) {
        this->getScreenSize(&screen_width, &screen_height);

        if (x == NIDIUM_WINDOWPOS_CENTER_MASK) {
            x = (screen_width - w) / 2;
        }
        if (y == NIDIUM_WINDOWPOS_CENTER_MASK) {
            y = (screen_height - h) / 2;
        }
    }

    if (x == NIDIUM_WINDOWPOS_UNDEFINED_MASK) {
        x = oldframe.origin.x;
    }
    if (y == NIDIUM_WINDOWPOS_UNDEFINED_MASK) {
        y = oldframe.origin.y;
    }

    CGRect newframe = [nswindow frameRectForContentRect:CGRectMake(x, y, w, h)];
    [nswindow setFrame:newframe display:YES];

    [pool drain];
}

#if 0
bool UICocoaInterface::makeMainGLCurrent()
{
    [((NSOpenGLContext *)m_mainGLCtx) makeCurrentContext];

    return true;
}

SDL_GLContext UICocoaInterface::getCurrentGLContext()
{
    return (SDL_GLContext)[NSOpenGLContext currentContext];
}
#endif

#if 0
bool UICocoaInterface::makeGLCurrent(SDL_GLContext ctx)
{
    [((NSOpenGLContext *)ctx) makeCurrentContext];

    return true;
}
#endif

static const char *drawRect_Associated_obj = "_UIInterface";

@interface NSPointer : NSObject
{
@public
    void *m_Ptr;
}

- (id) initWithPtr:(void *)ptr;
@end

@implementation NSPointer
- (id) initWithPtr:(void *)ptr
{
    self = [super init];
    if (!self) return nil;

    self->m_Ptr = ptr;

    return self;
}
@end
@interface NativeDrawRectResponder : NSView
    - (void) drawRect:(NSRect)dirtyRect;
@end

@implementation NativeDrawRectResponder
- (void) drawRect:(NSRect)dirtyRect
{
    NSPointer *idthis = objc_getAssociatedObject(self, drawRect_Associated_obj);
    UICocoaInterface *UI = (UICocoaInterface *)idthis->m_Ptr;
    Nidium::Frontend::Context *ctx = UI->getNativeContext();

    if (ctx && ctx->isSizeDirty()) {
        [(NSOpenGLContext *)UI->getGLContext() update];
        ctx->sizeChanged(UI->getWidth(), UI->getHeight());
    }
}
@end

void UICocoaInterface::patchSDLView(NSView *sdlview)
{
    Class SDLView = NSClassFromString(@"SDLView");
    Method drawRectNewMeth = class_getInstanceMethod([NativeDrawRectResponder class], @selector(drawRect:));
    IMP drawRectNewImp = method_getImplementation(drawRectNewMeth);
    const char *types = method_getTypeEncoding(drawRectNewMeth);

    class_replaceMethod(SDLView, @selector(drawRect:), drawRectNewImp, types);

    NSPointer *idthis = [[NSPointer alloc] initWithPtr:this];

    objc_setAssociatedObject(sdlview, drawRect_Associated_obj, idthis, OBJC_ASSOCIATION_RETAIN_NONATOMIC);

    [idthis release];
}

void UICocoaInterface::enableSysTray(const void *imgData,
    size_t imageDataSize)
{
#if 0
    m_StatusItem = [[[NSStatusBar systemStatusBar] statusItemWithLength:NSVariableStatusItemLength] retain];


    NSImage *icon = [NSApp applicationIconImage];
    [icon setScalesWhenResized:YES];
    [icon setSize: NSMakeSize(20.0, 20.0)];

    m_StatusItem.image = icon;
    m_StatusItem.highlightMode = YES;

    NSMenu *stackMenu = [[NSMenu alloc] initWithTitle:@""];

    NSMenuItem *menuOpen =
        [[NSMenuItem alloc] initWithTitle:@"Open" action:nil keyEquivalent:@""];
    NSMenuItem *menuClose =
        [[NSMenuItem alloc] initWithTitle:@"Close" action:nil keyEquivalent:@""];

    [stackMenu addItem:menuOpen];
    [stackMenu addItem:menuClose];

    [menuOpen setEnabled:YES];
    [menuClose setEnabled:YES];

    [m_StatusItem setMenu:stackMenu];
    m_StatusItem.title = @"";
#endif

    this->renderSystemTray();
}

void UICocoaInterface::disableSysTray()
{
    if (!m_StatusItem) {
        return;
    }
    [[NSStatusBar systemStatusBar] removeStatusItem:m_StatusItem];
    [m_StatusItem release];

    m_StatusItem = NULL;
}

void UICocoaInterface::quit()
{
    this->stopApplication();
    SDL_Quit();
    [[NSApplication sharedApplication] terminate:nil];
}

void UICocoaInterface::hideWindow()
{
    if (!m_Hidden) {
        m_Hidden = true;
        SDL_HideWindow(m_Win);

        /* Hide the Application (Dock, etc...) */
        [NSApp setActivationPolicy: NSApplicationActivationPolicyAccessory];

        set_timer_to_low_resolution(&this->m_Gnet->timersng, 1);
    }
}

void UICocoaInterface::showWindow()
{
    if (m_Hidden) {
        m_Hidden = false;
        SDL_ShowWindow(m_Win);

        [NSApp setActivationPolicy: NSApplicationActivationPolicyRegular];

        set_timer_to_low_resolution(&this->m_Gnet->timersng, 0);
    }
}

void UICocoaInterface::renderSystemTray()
{
    SystemMenuItem *item = m_SystemMenu.items();
    if (!item) {
        return;
    }

    if (!m_StatusItem) {
        m_StatusItem = [[[NSStatusBar systemStatusBar] statusItemWithLength:NSVariableStatusItemLength] retain];
        m_StatusItem.highlightMode = YES;

        size_t icon_len, icon_width, icon_height;
        const uint8_t *icon_custom = m_SystemMenu.getIcon(&icon_len,
                                        &icon_width, &icon_height);

        NSImage *icon;

        if (!icon_len || !icon_custom) {
            icon = [NSApp applicationIconImage];
            [icon setSize: NSMakeSize(20.0, 20.0)];
        } else {

            NSBitmapImageRep *rep = [[NSBitmapImageRep alloc] initWithBitmapDataPlanes:(unsigned char **)&icon_custom
                                                                            pixelsWide:icon_width
                                                                            pixelsHigh:icon_height
                                                                            bitsPerSample:8
                                                                            samplesPerPixel:4
                                                                            hasAlpha:YES
                                                                            isPlanar:NO
                                                                            colorSpaceName:NSDeviceRGBColorSpace
                                                                            bitmapFormat:NSAlphaNonpremultipliedBitmapFormat
                                                                            bytesPerRow:icon_width*4
                                                                            bitsPerPixel:32];

            icon = [[[NSImage alloc] initWithSize:NSMakeSize(32.f, 32.f)] autorelease];

            [icon addRepresentation:rep];
        }
        m_StatusItem.image = icon;
    }

    NSMenu *stackMenu = [m_Wrapper renderSystemTray];

    [m_StatusItem setMenu:stackMenu];
}

void UICocoaInterface::setSystemCursor(CURSOR_TYPE cursorvalue)
{
    switch(cursorvalue) {
        case UICocoaInterface::ARROW:
            [[NSCursor arrowCursor] set];
            break;
        case UICocoaInterface::BEAM:
            [[NSCursor IBeamCursor] set];
            break;
        case UICocoaInterface::CROSS:
            [[NSCursor crosshairCursor] set];
            break;
        case UICocoaInterface::POINTING:
            [[NSCursor pointingHandCursor] set];
            break;
        case UICocoaInterface::CLOSEDHAND:
            [[NSCursor closedHandCursor] set];
            break;
        case UICocoaInterface::RESIZELEFTRIGHT:
            [[NSCursor resizeLeftRightCursor] set];
            break;
        case UICocoaInterface::HIDDEN:
            SDL_ShowCursor(0);
            break;
        default:
            break;
    }
}

@implementation UICocoaInterfaceWrapper

- (id) initWithUI:(UICocoaInterface *)ui
{
    if (self = [super init]) {
        self->base = ui;
    }
    return self;
}

- (void) menuClicked:(id)sender
{
    NSString *identifier = [sender representedObject];

    JSWindow *window = JSWindow::GetObject(self->base->m_NativeCtx->getNJS());
    if (window) {
        window->systemMenuClicked([identifier cStringUsingEncoding:NSUTF8StringEncoding]);
    }
}

- (NSMenu *) renderSystemTray
{
    UICocoaInterface *ui = self->base;
    NativeSystemMenu &m_SystemMenu = ui->getSystemMenu();

    SystemMenuItem *item = m_SystemMenu.items();
    if (!item) {
        return nil;
    }

    NSMenu *stackMenu = [[[NSMenu alloc] initWithTitle:@""] retain];

    while (item) {
        NSString *title = [NSString stringWithCString:item->title() encoding:NSUTF8StringEncoding];
        NSString *identifier = [NSString stringWithCString:item->id() encoding:NSUTF8StringEncoding];
        NSMenuItem *curMenu;
        if ([title isEqualToString:@"-"]) {
            curMenu = [NSMenuItem separatorItem];
        } else {
            curMenu =
                [[[NSMenuItem alloc] initWithTitle:title action:@selector(menuClicked:) keyEquivalent:@""] autorelease];
        }
        [stackMenu addItem:curMenu];
        [curMenu setEnabled:YES];

        item = item->m_Next;
        curMenu.target = self;

        [curMenu setRepresentedObject:identifier];
    }

    return stackMenu;
}

@end
