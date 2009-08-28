/*
 * author: Onne Gorter <o.gorter@gmail.com>
 * package: antirsi-macosx
 * license: GPL
 */

#import <Cocoa/Cocoa.h>
#import "AntiRSIView.h"

#include "../antirsi-core/antirsi-core.h"

#define sLatestVersionURL @"http://tech.inhelsinki.nl/antirsi/antirsi_version.txt"
#define sURL @"http://tech.inhelsinki.nl/antirsi/"
#define sVersion @"2.0"

#define sMicroPause @"Micro Pause"
#define sWorkBreak  @"Work Break"

@interface AntiRSI : NSObject
{
    // views to display current status in
    IBOutlet AntiRSIView *view;
    IBOutlet NSProgressIndicator *progress;
    IBOutlet AntiRSIButton *postpone;
    IBOutlet NSTextField *label;
    IBOutlet NSTextField *time;
    IBOutlet NSTextField *next_break;
    IBOutlet NSTextField *version;

    IBOutlet NSMenuItem *menuBreakNow;
    IBOutlet NSMenuItem *dockBreakNow;
    IBOutlet NSMenuItem *menuPostpone;
    IBOutlet NSMenuItem *dockPostpone;

    // dock icon image
    NSImage* dock_image;
    NSImage* original_dock_image;

    // window to display the views in
    NSWindow *main_window;

    // timer that ticks every second to update
    NSTimer *mtimer;

    double sample_interval;

    // verious other options
    bool lock_focus;
    bool draw_dock_image;
    bool draw_dock_image_q;

    // various colors
    NSColor* taking;
    NSColor* elapsed;
    NSColor* background;
    NSColor* darkbackground;

    ai_core * core;
}

//bindings
- (void)setSample_interval:(NSString *)s;
- (void)setDraw_dock_image:(BOOL)b;
- (void)setBackground:(NSColor *)c;

// goto website button
- (IBAction)gotoWebsite:(id)sender;

// check updates
- (IBAction)checkForUpdate:(id)sender;

// postpone button
- (IBAction)postpone:(id)sender;

// workbreak now menu item
- (IBAction)breakNow:(id)sender;

// one second ticks away ...
- (void)tick:(NSTimer *)timer;

// update break window
- (void)drawBreakWindow;

// draw the dock icon
- (void)drawDockImage;

// run the micro pause window
- (void)doMicroPause;

// run the work break window
- (void)doWorkBreak;

// stop micro pause or work break
- (void)endBreak;

// time left string
- (void)drawTimeLeft:(int)seconds;

// time to next break string
- (void)drawNextBreak:(int)seconds;

@end

