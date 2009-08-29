/*
 * author: Onne Gorter <o.gorter@gmail.com>
 * package: antirsi-macosx
 * license: GPL
 *
 * TODO
 * clean up from old antirsi, like mini_break was micro_pause ...
 * remove smooth preference thingy, its no longer 600 mhz computers and old macosx
 * investigate status bar
 * use new style dockicon api
 */

#import "AntiRSI.h"

#include <math.h>
#include <ApplicationServices/ApplicationServices.h>

// entry functions from antirsi-core

static void handle_break_end(void * data) {
    id ai = (id)data;
    [ai endBreak];
}

static void handle_mini_break_start(void * data) {
    id ai = (id)data;
    [ai doMicroPause];
}

static void handle_work_break_start(void * data) {
    id ai = (id)data;
    [ai doWorkBreak];
}

static void handle_break_update(void * data) {
    id ai = (id)data;
    [ai drawBreakWindow];
}

static void handle_status_update(void * data) {
    id ai = (id)data;
    [ai drawDockImage];
}

@implementation AntiRSI

// bindings methods
- (void)setMicro_pause_duration:(float)f { core->mini_duration = round(f); }
- (void)setMicro_pause_period:(float)f   { core->mini_interval = 60 * round(f); }
- (void)setWork_break_duration:(float)f  { core->work_duration = 60 * round(f); }
- (void)setWork_break_period:(float)f    { core->work_interval = 60 * round(f); }

- (float)micro_pause_duration { return core->mini_duration; }
- (float)micro_pause_period   { return core->mini_interval; }
- (float)work_break_duration  { return core->work_duration; }
- (float)work_break_period    { return core->work_interval; }

- (void)installTimer:(double)interval
{
    if (mtimer != nil) {
        [mtimer invalidate];
        [mtimer autorelease];
    }
    mtimer = [[NSTimer scheduledTimerWithTimeInterval:interval
                                               target:self
                                             selector:@selector(tick:)
                                             userInfo:nil repeats:YES] retain];
}

- (void)setSample_interval:(NSString *)s
{
    sample_interval = 1;
    if ([s isEqualToString:@"Super Smooth"]) sample_interval = 0.1;
    if ([s isEqualToString:@"Smooth"]) sample_interval = 0.33;
    if ([s isEqualToString:@"Normal"]) sample_interval = 1;
    if ([s isEqualToString:@"Low"]) sample_interval = 2;

    [self installTimer:sample_interval];
}

- (void)setDraw_dock_image:(BOOL)b
{
    draw_dock_image=b;
    if (!b) {
        [NSApp setApplicationIconImage: original_dock_image];
    } else {
        [self drawDockImage];
    }
}

- (void)setBackground:(NSColor *)c
{
    [background autorelease];
    background=[c retain];

    // make new darkbackground color
    float r,g,b,a;
    [background getRed:&r green:&g blue:&b alpha:&a];
    [darkbackground autorelease];
    darkbackground=[[NSColor colorWithCalibratedRed:r*0.35 green:g*0.35 blue:b*0.35 alpha:a+0.2] retain];

    [self drawDockImage];
}

- (void)setElapsed:(NSColor *)c
{
    [elapsed autorelease];
    elapsed=[c retain];
    [self drawDockImage];
}

- (void)setTaking:(NSColor *)c
{
    [taking autorelease];
    taking=[c retain];
    [self drawDockImage];
}

// end of bindings

- (void)awakeFromNib
{
    core = antirsi_init(self);
    // want transparancy
    [NSColor setIgnoresAlpha:NO];

    // initial colors
    elapsed = [[NSColor colorWithCalibratedRed:0.3 green:0.3 blue:0.9 alpha:0.95] retain];
    taking = [[NSColor colorWithCalibratedRed:0.3 green:0.9 blue:0.3 alpha:0.90] retain];
    background = [NSColor colorWithCalibratedRed:0.9 green:0.9 blue:0.9 alpha:0.7];

    //initial values
    core->mini_interval = 4*60;
    core->mini_duration = 13;
    core->work_interval = 50*60;
    core->work_duration = 8*60;

    core->emit_break_end = handle_break_end;
    core->emit_mini_break_start = handle_mini_break_start;
    core->emit_work_break_start = handle_work_break_start;
    core->emit_break_update = handle_break_update;
    core->emit_status_update = handle_status_update;

    sample_interval = 1;

    // initialize dock image
    dock_image = [[NSImage alloc] initWithSize:NSMakeSize(128,128)];
    [dock_image setCacheMode:NSImageCacheNever];
    original_dock_image = [NSImage imageNamed:@"AntiRSI"];
    draw_dock_image_q = YES;

    // setup main window that will show either micropause or workbreak
    main_window = [[NSWindow alloc] initWithContentRect:[view frame]
                                              styleMask:NSBorderlessWindowMask
                                                backing:NSBackingStoreBuffered defer:YES];
    [main_window setBackgroundColor:[NSColor clearColor]];
    [main_window setLevel:NSScreenSaverWindowLevel];
    [main_window setAlphaValue:0.85];
    [main_window setOpaque:NO];
    [main_window setHasShadow:NO];
    [main_window setMovableByWindowBackground:YES];
    [main_window center];
    [main_window setContentView:view];
    [main_window setCollectionBehavior:NSWindowCollectionBehaviorCanJoinAllSpaces];

    // set background now
    [self setBackground:background];

    // create initial values
    NSMutableDictionary* initial = [NSMutableDictionary dictionaryWithCapacity:10];
    [initial setObject:[NSNumber numberWithFloat:4] forKey:@"micro_pause_period"];
    [initial setObject:[NSNumber numberWithFloat:13] forKey:@"micro_pause_duration"];
    [initial setObject:[NSNumber numberWithFloat:50] forKey:@"work_break_period"];
    [initial setObject:[NSNumber numberWithFloat:8] forKey:@"work_break_duration"];
    [initial setObject:@"Smooth" forKey:@"sample_interval"];
    [initial setObject:[NSNumber numberWithBool:YES] forKey:@"draw_dock_image"];
    [initial setObject:[NSNumber numberWithBool:NO] forKey:@"lock_focus"];
    [initial setObject:[NSArchiver archivedDataWithRootObject:elapsed] forKey:@"elapsed"];
    [initial setObject:[NSArchiver archivedDataWithRootObject:taking] forKey:@"taking"];
    [initial setObject:[NSArchiver archivedDataWithRootObject:background] forKey:@"background"];
    [[NSUserDefaultsController sharedUserDefaultsController] setInitialValues:initial];

    // bind to defauls controller
    id dc = [NSUserDefaultsController sharedUserDefaultsController];
    [self bind:@"micro_pause_period" toObject:dc withKeyPath:@"values.micro_pause_period" options:nil];
    [self bind:@"micro_pause_duration" toObject:dc withKeyPath:@"values.micro_pause_duration" options:nil];
    [self bind:@"work_break_period" toObject:dc withKeyPath:@"values.work_break_period" options:nil];
    [self bind:@"work_break_duration" toObject:dc withKeyPath:@"values.work_break_duration" options:nil];
    [self bind:@"sample_interval" toObject:dc withKeyPath:@"values.sample_interval" options:nil];
    [self bind:@"draw_dock_image" toObject:dc withKeyPath:@"values.draw_dock_image" options:nil];
    [self bind:@"lock_focus" toObject:dc withKeyPath:@"values.lock_focus" options:nil];
    NSDictionary* unarchive = [NSDictionary dictionaryWithObject:NSUnarchiveFromDataTransformerName forKey:@"NSValueTransformerName"];
    [self bind:@"elapsed" toObject:dc withKeyPath:@"values.elapsed" options:unarchive];
    [self bind:@"taking" toObject:dc withKeyPath:@"values.taking" options:unarchive];
    [self bind:@"background" toObject:dc withKeyPath:@"values.background" options:unarchive];

    // alert every binding
    [[NSUserDefaultsController sharedUserDefaultsController] revert:self];

    // start the timer
    [self installTimer:sample_interval];

    // about dialog
    sVersion = [[NSString stringWithFormat:@"%@", [[[NSBundle mainBundle] infoDictionary] objectForKey:@"CFBundleVersion"]] retain];
    [version setStringValue:[NSString stringWithFormat:@"Version %@", sVersion]];

    // TODO remove?
    [progress setMaxValue:1];
}

// tick every second and update status
- (void)tick:(NSTimer *)timer {

    // still even iTunes posts HID events to prevent screen blanks and screen saver ... ugly
    CFTimeInterval idle_time = CGEventSourceSecondsSinceLastEventType(kCGEventSourceStateHIDSystemState, kCGAnyInputEventType);
    ai_tick(core, idle_time);
}

// draw the break window progress bar and such
- (void)drawBreakWindow {
    // update window
    [progress setDoubleValue:ai_break_progress(core)];
    [self drawTimeLeft:ai_break_time_left(core)];
    [self drawNextBreak:ai_seconds_until_next_work_break(core)];

    // if user likes to be interrupted
    if (lock_focus) {
        [NSApp activateIgnoringOtherApps:YES];
        [main_window makeKeyAndOrderFront:self];
    }
}

// draw the dock icon
- (void)drawDockImage {
    if (!draw_dock_image) return;

    [dock_image lockFocus];

    // clear all
    [[NSColor clearColor] set];
    NSRectFill(NSMakeRect(0,0,127,127));

    NSBezierPath* p;
    float end;

    //draw background circle
    [darkbackground set];
    p =[NSBezierPath bezierPathWithOvalInRect:NSMakeRect(6,6,115,115)];
    [p setLineWidth:4];
    [p stroke];

    //fill
    [background set];
    [[NSBezierPath bezierPathWithOvalInRect:NSMakeRect(8,8,111,111)] fill];

    //put dot in middle
    [darkbackground set];
    [[NSBezierPath bezierPathWithOvalInRect:NSMakeRect(59,59,9,9)] fill];

    // reuse this one
    p = [NSBezierPath bezierPath];

    // draw work_break
    [elapsed set];
    end = 360 - (360.0 / core->work_interval * core->work_t - 90);
    if (end <= 90) end=90.1;
    [p appendBezierPathWithArcWithCenter:NSMakePoint(63.5, 63.5) radius:40 startAngle:90 endAngle:end clockwise:YES];
    [p setLineWidth:22];
    [p stroke];

    // draw work break taking
    [taking set];
    [p removeAllPoints];
    end = 360 - (360.0 / core->work_duration * core->work_taking_t - 90);
    if (end <= 90) end=90.1;
    [p appendBezierPathWithArcWithCenter:NSMakePoint(63.5, 63.5) radius:40 startAngle:90 endAngle:end clockwise:YES];
    [p setLineWidth:18];
    [p stroke];

    // draw micro pause
    [elapsed set];
    [p removeAllPoints];
    end = 360 - (360.0 / core->mini_interval * core->mini_t - 90);
    if (end <= 90) end = 90.1;
    [p appendBezierPathWithArcWithCenter:NSMakePoint(63.5, 63.5) radius:17 startAngle:90 endAngle:end clockwise:YES];
    [p setLineWidth:22];
    [p stroke];

    // draw micro pause taking
    [taking set];
    [p removeAllPoints];
    end = 360 - (360.0 / core->mini_duration * core->mini_taking_t - 90);
    if (end <= 90) end = 90.1;
    [p appendBezierPathWithArcWithCenter:NSMakePoint(63.5, 63.5) radius:17 startAngle:90 endAngle:end clockwise:YES];
    [p setLineWidth:18];
    [p stroke];

    [dock_image unlockFocus];

    // and set it in the dock check draw_dock_image one last time ...
    if (draw_dock_image_q) [NSApp setApplicationIconImage:dock_image];
}

// done with micro pause or work break
- (void)endBreak {
    [[main_window animator] setAlphaValue:0.0];
    // what is the consequence of hiding it, instead of ordering it out??
    //[main_window orderOut:NULL];

    // reset time interval to user's choice
    [self installTimer:sample_interval];
}

// center and make appear the break window
- (void)orderInBreakWindow {
    [main_window center];
    [main_window orderFrontRegardless];
    [main_window setAlphaValue:0.0];
    [[main_window animator] setAlphaValue:1.0];

    // temporarily set time interval for smooth updating during the pause
    [self installTimer:0.1];
}

// display micro_pause window with appropriate widgets and progress bar
- (void)doMicroPause {
    [label setStringValue: sMicroPause];
    [progress setDoubleValue:ai_break_progress(core)];
    [postpone setHidden:YES];
    [self drawTimeLeft:ai_break_time_left(core)];
    [self drawNextBreak:ai_seconds_until_next_work_break(core)];
    [self orderInBreakWindow];
}

// display work_break window with appropriate widgets and progress bar
- (void)doWorkBreak {
    [label setStringValue: sWorkBreak];
    [progress setDoubleValue:ai_break_progress(core)];
    [postpone setHidden:NO];
    [self drawTimeLeft:ai_break_time_left(core)];
    [self drawNextBreak:ai_seconds_until_next_work_break(core)];
    [self orderInBreakWindow];
}

// diplays time left
- (void)drawTimeLeft:(int)seconds {
    [time setStringValue:[NSString stringWithFormat:@"%d:%02d", seconds / 60, seconds % 60]];
}

// displays next break
- (void)drawNextBreak:(int)seconds {
    int minutes = round(seconds / 60.0) ;

    // nice hours, minutes ...
    if (minutes > 60) {
        [next_break setStringValue:[NSString stringWithFormat:@"next break in %d:%02d hours",
            minutes / 60, minutes % 60]];
    } else {
        [next_break setStringValue:[NSString stringWithFormat:@"next break in %d minutes", minutes]];
    }
}

// goto website
- (IBAction)gotoWebsite:(id)sender
{
    [[NSWorkspace sharedWorkspace] openURL:[NSURL URLWithString:sURL]];
}

// check for update
- (IBAction)checkForUpdate:(id)sender
{
    NSString *latest_version = [NSString stringWithContentsOfURL: [NSURL URLWithString:sLatestVersionURL]];
    if (latest_version == Nil) latest_version = @"";
    
    latest_version = [latest_version stringByTrimmingCharactersInSet:[NSCharacterSet whitespaceAndNewlineCharacterSet]];

    if ([latest_version length] == 0) {
        NSRunInformationalAlertPanel(
            @"Unable to Determine",
            @"Unable to determine the latest AntiRSI version number.",
            @"Ok", nil, nil);
    } else if ([latest_version compare:sVersion] == NSOrderedDescending) {
        int r = NSRunInformationalAlertPanel(
            @"New Version",
            [NSString stringWithFormat:@"A new version (%@) of AntiRSI is available; would you like to go to the website now?", latest_version],
            @"Goto Website", @"Cancel", nil);
        if (r == NSOKButton) {
            [[NSWorkspace sharedWorkspace] openURL:[NSURL URLWithString:sURL]];
        }
    } else {
        NSRunInformationalAlertPanel(
            @"No Update Available",
            @"This is the latest version of AntiRSI.",
            @"OK", nil, nil);
    }
}

// stop work break and postpone by 10 minutes
- (IBAction)postpone:(id)sender {
    ai_work_break_postpone(core);
}

// start a work break right now
- (IBAction)breakNow:(id)sender {
    ai_work_break_now(core);
}

// validate menu items
- (BOOL)validateMenuItem:(NSMenuItem *)menu {

    if (menu == menuBreakNow || menu == dockBreakNow) {
        if (ai_can_continue_natural_break(core)) {
            [menu setTitle: @"Continue Work Break"];
        } else {
            [menu setTitle: @"Take Break Now"];
        }
        return core->state == S_NORMAL;
    }

    if (menu == menuPostpone || menu == dockPostpone) {
        return core->state == S_IN_WORK;
    }

    return [super validateMenuItem:menu];
}

- (void)applicationWillTerminate:(NSNotification *)aNotification
{
    // make sure timer doesn't tick once more ...
    draw_dock_image_q = NO;
    [mtimer invalidate];
    [mtimer autorelease];
    mtimer = nil;
    [dock_image release];

    // and make sure to show original dock image
    [NSApp setApplicationIconImage: original_dock_image];
}

@end

