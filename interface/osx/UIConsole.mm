/*
   Copyright 2016 Nidium Inc. All rights reserved.
   Use of this source code is governed by a MIT license
   that can be found in the LICENSE file.
*/
#import "UIConsole.h"


@implementation NativeConsole

@synthesize m_Window, textview, m_IsHidden;

- (id) init
{
    if (!(self = [super init])) return nil;

    self.m_IsHidden = NO;

    self.m_Window = [[NSWindow alloc] initWithContentRect:NSMakeRect(100, 100, 700, 480) styleMask: NSTitledWindowMask backing:NSBackingStoreBuffered defer:NO];

    [self.m_Window setFrameAutosaveName:@"nativeConsole"];
    [self.m_Window setTitle:@"nidium console"];
    CGRect frame = [[self.m_Window contentView] frame];
    NSButton *btn = [[NSButton alloc] initWithFrame:NSMakeRect(10, 3, 100, 25)];
    [btn setButtonType:NSMomentaryLightButton]; //Set what type button You want
    [btn setBezelStyle:NSRegularSquareBezelStyle]; //Set what style You want
    btn.title = @"Clear";
    [btn setTarget:self];
    [btn setAction:@selector(clearPressed)];

    frame.origin.y = 32;
    frame.size.height -= 32;

    NSView *mview = [[NSView alloc] initWithFrame:frame];
    [mview setWantsLayer:YES];

    NSScrollView *scrollview = [[NSScrollView alloc]

                                initWithFrame:frame];

    [mview addSubview:scrollview];

    NSSize contentSize = [scrollview contentSize];

    [mview addSubview:btn];

    [scrollview setBorderType:NSNoBorder];

    [scrollview setHasVerticalScroller:YES];

    [scrollview setHasHorizontalScroller:NO];

    [scrollview setAutoresizingMask:NSViewWidthSizable];

    self.textview = [[NSTextView alloc] initWithFrame:NSMakeRect(0,
                                                                 0,
                                                                 contentSize.width,
                                                                 contentSize.height)];

    [textview setMinSize:NSMakeSize(0.0, contentSize.height)];

    [textview setMaxSize:NSMakeSize(FLT_MAX, FLT_MAX)];

    [textview setVerticallyResizable:YES];

    [textview setHorizontallyResizable:NO];
    [[textview layoutManager] setBackgroundLayoutEnabled:YES];
    [[textview layoutManager] setAllowsNonContiguousLayout:YES];

    [textview setAutoresizingMask:NSViewWidthSizable];

    [[textview textContainer] setContainerSize:NSMakeSize(contentSize.width, FLT_MAX)];

    [[textview textContainer] setWidthTracksTextView:YES];

    [scrollview setDocumentView:textview];

    [m_Window setContentView:mview];

    [m_Window makeKeyAndOrderFront:nil];

    [m_Window makeFirstResponder:textview];

    [self.m_Window setContentBorderThickness:32.0 forEdge:NSMinYEdge];
    [self.m_Window release];
    [self.textview release];

    [textview insertText:@"Console ready.\n"];
    [textview setBackgroundColor:[NSColor blackColor]];
    [textview setTextColor:[NSColor greenColor]];
    [textview setFont:[NSFont fontWithName:@"Monaco" size:10]];

    return self;
}

-(void)clearPressed {
    [self clear];
}

- (void) log:(NSString *)str
{
    if (!self.m_IsHidden) {
        @try {
            [[[textview textStorage] mutableString] appendString: str];
        } @catch(NSException *e) {}
    }
}

- (void) clear
{
    [textview setString:@""];
    @try {
        [textview insertText:@"Console ready.\n"];
    } @catch(NSException *e){}

    [textview setFont:[NSFont fontWithName:@"Monaco" size:10]];
}

- (void) attachToStdout
{
    NSPipe *pipe = [NSPipe pipe];
    NSFileHandle *pipeReadHandle = [pipe fileHandleForReading];

    dup2([[pipe fileHandleForWriting] fileDescriptor], fileno(stdout));

    [[NSNotificationCenter defaultCenter] addObserver:self
                                             selector:@selector(handleNotification:)
                                                 name:NSFileHandleReadCompletionNotification
                                               object:pipeReadHandle];
    [pipeReadHandle readInBackgroundAndNotify];

}

- (void) handleNotification:(NSNotification *)notification
{
    NSFileHandle *pipeReadHandle = notification.object;
    [pipeReadHandle readInBackgroundAndNotify];
    NSString *str = [[NSString alloc] initWithData: [[notification userInfo] objectForKey: NSFileHandleNotificationDataItem] encoding: NSASCIIStringEncoding];

    [self log:str];

}

@end

UICocoaConsole::UICocoaConsole()
{
    this->m_Window = [[NativeConsole alloc] init];
    this->m_NeedFlush = false;
    //[this->m_Window attachToStdout];
    this->show();
    //this->hide();
}

void UICocoaConsole::clear()
{
    if (![NSThread isMainThread]) {
        dispatch_async(dispatch_get_main_queue(), ^{
            this->flush();
            [this->m_Window clear];
        });
    } else {
        this->flush();
        [this->m_Window clear];
    }

}

void UICocoaConsole::flush()
{
    if (m_NeedFlush) {
        [[[this->m_Window textview] textStorage] endEditing];
        [[this->m_Window textview] scrollRangeToVisible: NSMakeRange ([[this->m_Window.textview string] length], 0)];
        m_NeedFlush = false;
    }
}

void UICocoaConsole::hide()
{
    if (this->m_IsHidden) {
        return;
    }
    [this->m_Window.m_Window orderOut:nil];
    this->m_IsHidden = true;
    [this->m_Window setIsHidden:YES];
}

bool UICocoaConsole::hidden()
{
    return this->m_IsHidden;
}

void UICocoaConsole::show()
{
    if (!this->m_IsHidden) {
        return;
    }
    [this->m_Window.m_Window orderFront:nil];
    this->m_IsHidden = false;
    [this->m_Window setIsHidden:NO];
}

void UICocoaConsole::log(const char *str)
{
    char *copy_str = strdup(str);
    typedef void(^_closure)();

    _closure func = ^{
        if (!m_NeedFlush) {
            [[[this->m_Window textview] textStorage] beginEditing];
            m_NeedFlush = true;
        }
        if (this->m_IsHidden) {
            return;
        }
        NSString *nstr = [NSString stringWithCString:copy_str encoding:NSUTF8StringEncoding];
        [this->m_Window log:nstr];

        free(copy_str);
    };

    if (![NSThread isMainThread]) {
        dispatch_async(dispatch_get_main_queue(), ^{
            func();
        });
    } else {
        func();
    }
}

UICocoaConsole::~UICocoaConsole()
{
    [this->m_Window release];
}
