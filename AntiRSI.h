/*
 author: Onne Gorter
 
 This file is part of AntiRSI.
 
 AntiRSI is free software; you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation; either version 2 of the License, or
 (at your option) any later version.
 
 AntiRSI is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.
 
 You should have received a copy of the GNU General Public License
 along with AntiRSI; if not, write to the Free Software
 Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#import <Cocoa/Cocoa.h>
#import "AntiRSIView.h"
#import "CTBadge.h"

#define sLatestVersionURL @"http://web.sabi.net/nriley/software/AntiRSI-version.txt"
#define sURL @"http://web.sabi.net/nriley/software/#antirsi"
#define sVersion @"1.4njr4"

typedef enum _AntiRSIState {
    s_normal = 0,
    s_taking_micro_pause,
    s_taking_work_break,
} AntiRSIState;

@interface AntiRSI : NSObject
{
    // views to display current status in
    IBOutlet AntiRSIView *view;
    IBOutlet NSLevelIndicator *progress;
    IBOutlet NSButton *postpone;
    IBOutlet NSTextField *time;
    IBOutlet NSTextField *next_break;
    IBOutlet NSTextField *session_time;
    IBOutlet NSTextField *status;
    IBOutlet NSTextField *version; // XXX unused?
    IBOutlet NSDatePicker *reset_session_time;
    
    // dock menu
    IBOutlet NSMenu *dock_menu;
    IBOutlet NSMenuItem *session_time_item;
    
    // dock icon image
    NSImage* dock_image;
    NSImage* original_dock_image;
    CTBadge* dock_badge;
	
    // window to display the views in
    NSWindow *main_window;
    
    // timer that ticks every second to update
    NSTimer *mtimer;
    
    // various timers
    double micro_pause_t;
    double work_break_t;
    double micro_pause_taking_t;
    double work_break_taking_t;
    double work_break_taking_cached_t;
    double work_break_taking_cached_date;
    double session_t;
    double date;
    double reset_session_date;
		
    // various timing lengths
    int micro_pause_period;
    int micro_pause_duration;
    int work_break_period;
    int work_break_duration;
    
    double sample_interval;
    
    // various other options
    bool lock_focus;
    bool draw_dock_image;
    bool draw_dock_badge;
    bool draw_dock_image_q;
    bool reset_session_timer_daily;
    bool reset_session_timer_after;
    NSCalendarDate *reset_session_timer_time;
    int reset_session_timer_after_hours;
	
    // various colors
    NSColor* taking;
    NSColor* elapsed;
    NSColor* background;
    NSColor* darkbackground;
    
    // state we are in
    AntiRSIState state;
    
    // history filter
    double h0;
    double h1;
    double h2;
}

//bindings
- (void)setMicro_pause_duration:(float)f;
- (void)setMicro_pause_period:(float)f;
- (void)setWork_break_period:(float)f;
- (void)setWork_break_period:(float)f;
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

// reset session time menu item
- (IBAction)resetSession:(id)sender;

// returns string of the form "Session: 12:34:56"
- (NSString *)sessionTimeString;

// one second ticks away ...
- (void)tick:(NSTimer *)timer;

// reset all timers
- (void)resetTimers;

// draw the dock icon
- (void)drawDockImage;

// run the micro pause window
- (void)doMicroPause;

// run the work break window
- (void)doWorkBreak;

// stop micro pause or work break
- (void)endBreak;

// update window
- (void)updateBreakWindowDuration:(double)duration progress:(double)progress_t nextBreak:(double)nextBreak;

@end