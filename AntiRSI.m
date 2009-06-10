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

#import "AntiRSI.h"

#include <math.h>
#include <ApplicationServices/ApplicationServices.h>

// 10.5-only
enum {
    NSWindowCollectionBehaviorDefault = 0,
    NSWindowCollectionBehaviorCanJoinAllSpaces = 1 << 0,
    NSWindowCollectionBehaviorMoveToActiveSpace = 1 << 1
};
typedef unsigned NSWindowCollectionBehavior;
@interface NSWindow (NSWindowCollectionBehavior)
- (void)setCollectionBehavior:(NSWindowCollectionBehavior)behavior;
@end

extern CFTimeInterval CGSSecondsSinceLastInputEvent(unsigned long eventType);

static int badge_session_minutes = -1;

@implementation AntiRSI

- (void)computeResetSessionDate;
{
    NSCalendarDate *now = [NSCalendarDate calendarDate];
    NSCalendarDate *reset =
    [NSCalendarDate dateWithYear:[now yearOfCommonEra]
			   month:[now monthOfYear]
			     day:[now dayOfMonth]
			    hour:[reset_session_timer_time hourOfDay]
			  minute:[reset_session_timer_time minuteOfHour]
			  second:0
			timeZone:[NSTimeZone systemTimeZone]];
    if ([now compare:reset] != NSOrderedAscending)
        reset = [reset dateByAddingYears:0 months:0 days:1 hours:0 minutes:0 seconds:0];
    reset_session_date = [reset timeIntervalSinceReferenceDate];
}

// bindings methods
- (void)setMicro_pause_duration:(float)f
{
    micro_pause_duration = round(f);
    if (s_taking_micro_pause == state) {
	[progress setMaxValue:micro_pause_duration];
	[progress setDoubleValue:micro_pause_taking_t];
    }
}

- (void)setMicro_pause_period:(float)f
{	micro_pause_period = 60 * round(f); }

- (void)setWork_break_duration:(float)f
{   
    work_break_duration = 60 * round(f); 
    if (s_taking_work_break == state) {
	[progress setMaxValue:work_break_duration / 60];
	[progress setDoubleValue:work_break_taking_t / 60 - 0.5];
    }
}

- (void)setWork_break_period:(float)f
{	work_break_period = 60 * round(f); }

- (void)setReset_session_timer_time:(NSDate *)d;
{
    if (reset_session_timer_time != nil)
        [reset_session_timer_time autorelease];
    reset_session_timer_time = [[NSCalendarDate alloc] initWithTimeIntervalSinceReferenceDate:[d timeIntervalSinceReferenceDate]];
    [reset_session_timer_time setTimeZone:[NSTimeZone timeZoneForSecondsFromGMT:0]];
    [self computeResetSessionDate];
}

- (void)installTimer:(double)interval
{
    if (mtimer != nil) {
	[mtimer invalidate];
	[mtimer autorelease];
    }
    mtimer = [[NSTimer scheduledTimerWithTimeInterval:interval target:self selector:@selector(tick:)
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
	[NSApp setApplicationIconImage:[NSImage imageNamed:@"AntiRSI"]];
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
    [dock_badge setBadgeColor:elapsed];
    badge_session_minutes = -1;
    [self drawDockImage];
}

- (void)setTaking:(NSColor *)c
{
    [taking autorelease];
    taking=[c retain];
    [self drawDockImage];
}

// end of bindings

- (void)resetTimers;
{
    // set timers to 0
    micro_pause_t = 0;
    work_break_t = 0;
    micro_pause_taking_t = 0;
    work_break_taking_t = 0;
    work_break_taking_cached_t = 0;
    work_break_taking_cached_date = 0;
    session_t = 0;
}

- (void)awakeFromNib
{
    // want transparancy
    [NSColor setIgnoresAlpha:NO];
    
    // initial colors
    elapsed = [[NSColor colorWithCalibratedRed:0.3 green:0.3 blue:0.9 alpha:0.95] retain];
    taking = [[NSColor colorWithCalibratedRed:0.3 green:0.9 blue:0.3 alpha:0.90] retain];
    background = [NSColor colorWithCalibratedRed:0.9 green:0.9 blue:0.9 alpha:0.7];
    
    //initial values
    micro_pause_period = 4*60;
    micro_pause_duration = 13;
    work_break_period = 50*60;
    work_break_duration = 8*60;
    sample_interval = 1;
    
    // set current state
    state = s_normal;
    [self resetTimers];
    
    // initialize dock image
    dock_image = [[NSImage alloc] initWithSize:NSMakeSize(128,128)];
    [dock_image setCacheMode:NSImageCacheNever];
    original_dock_image = [NSImage imageNamed:@"AntiRSI"];
    draw_dock_image_q = YES;
    dock_badge = [[CTBadge systemBadge] retain];
    [dock_badge setBadgeColor:elapsed];
    
    // setup main window that will show either micropause or workbreak
    main_window = [[NSWindow alloc] initWithContentRect:[view frame]
					      styleMask:NSBorderlessWindowMask
						backing:NSBackingStoreBuffered defer:YES];
    [main_window setBackgroundColor:[NSColor clearColor]];
    [main_window setLevel:NSStatusWindowLevel];
    [main_window setAlphaValue:0.85];
    [main_window setOpaque:NO];
    [main_window setHasShadow:NO];
    [main_window setMovableByWindowBackground:YES];
    if ([main_window respondsToSelector:@selector(setCollectionBehavior:)])
	[main_window setCollectionBehavior: NSWindowCollectionBehaviorCanJoinAllSpaces];

    [main_window center];
    [main_window setContentView:view];
    NSTimeZone *utcZone = [NSTimeZone timeZoneForSecondsFromGMT:0];
    [reset_session_time setTimeZone:utcZone];
    [progress setEnabled:NO];
    NSFont *myriad = [NSFont fontWithName: @"Myriad" size: 40];
    if (myriad) [status setFont: myriad];
    
    // initialze history filter
    h0 = 0;
    h1 = 0;
    h2 = 0;
    
    // initialize ticks
    date = [NSDate timeIntervalSinceReferenceDate];
    
    // set background now
    [self setBackground:background];
    
    // create initial values
    NSUserDefaultsController *dc = [NSUserDefaultsController sharedUserDefaultsController];
    NSMutableDictionary* initial = [NSMutableDictionary dictionaryWithObjectsAndKeys:
				    [NSNumber numberWithFloat:4], @"micro_pause_period",
				    [NSNumber numberWithFloat:13], @"micro_pause_duration",
				    [NSNumber numberWithFloat:50], @"work_break_period",
				    [NSNumber numberWithFloat:8], @"work_break_duration",
				    @"Smooth", @"sample_interval",
				    [NSNumber numberWithBool:YES], @"draw_dock_image",
				    [NSNumber numberWithBool:YES], @"draw_dock_badge",
				    [NSNumber numberWithBool:NO], @"lock_focus",
				    [NSNumber numberWithBool:NO], @"reset_session_timer_daily",
				    [NSNumber numberWithBool:NO], @"reset_session_timer_after",
				    [NSCalendarDate dateWithYear:2000 month:1 day:1 hour:6 minute:0 second:0 timeZone:utcZone], @"reset_session_timer_time",
				    [NSNumber numberWithInt:8], @"reset_session_timer_after_hours",
				    [NSArchiver archivedDataWithRootObject:elapsed], @"elapsed",
				    [NSArchiver archivedDataWithRootObject:taking], @"taking",
				    [NSArchiver archivedDataWithRootObject:background], @"background",
				    nil];
    [[NSUserDefaults standardUserDefaults] registerDefaults:initial];
    [dc setInitialValues:initial];
    
    // bind to defaults controller
    [self bind:@"micro_pause_period" toObject:dc withKeyPath:@"values.micro_pause_period" options:nil];
    [self bind:@"micro_pause_duration" toObject:dc withKeyPath:@"values.micro_pause_duration" options:nil];
    [self bind:@"work_break_period" toObject:dc withKeyPath:@"values.work_break_period" options:nil];
    [self bind:@"work_break_duration" toObject:dc withKeyPath:@"values.work_break_duration" options:nil];
    [self bind:@"sample_interval" toObject:dc withKeyPath:@"values.sample_interval" options:nil];
    [self bind:@"draw_dock_image" toObject:dc withKeyPath:@"values.draw_dock_image" options:nil];
    [self bind:@"draw_dock_badge" toObject:dc withKeyPath:@"values.draw_dock_badge" options:nil];
    [self bind:@"lock_focus" toObject:dc withKeyPath:@"values.lock_focus" options:nil];
    [self bind:@"reset_session_timer_daily" toObject:dc withKeyPath:@"values.reset_session_timer_daily" options:nil];
    [self bind:@"reset_session_timer_after" toObject:dc withKeyPath:@"values.reset_session_timer_after" options:nil];
    [self bind:@"reset_session_timer_time" toObject:dc withKeyPath:@"values.reset_session_timer_time" options:nil];
    [self bind:@"reset_session_timer_after_hours" toObject:dc withKeyPath:@"values.reset_session_timer_after_hours" options:nil];
    NSDictionary* unarchive = [NSDictionary dictionaryWithObject:NSUnarchiveFromDataTransformerName forKey:@"NSValueTransformerName"];
    [self bind:@"elapsed" toObject:dc withKeyPath:@"values.elapsed" options:unarchive];
    [self bind:@"taking" toObject:dc withKeyPath:@"values.taking" options:unarchive];
    [self bind:@"background" toObject:dc withKeyPath:@"values.background" options:unarchive];
    
    // alert every binding
    [dc revert:self];
    
    // start the timer
    [self installTimer:sample_interval];
    
    // about dialog
    [version setStringValue:[NSString stringWithFormat:@"Version %@", sVersion]]; 
}

// tick every second and update status
- (void)tick:(NSTimer *)timer
{
    // calculate time since last tick
    NSTimeInterval new_date = [NSDate timeIntervalSinceReferenceDate];
    NSTimeInterval tick_time = new_date - date;
    date = new_date;
    
    if (reset_session_timer_daily && date >= reset_session_date) {
        [self resetSession:nil];
        [self computeResetSessionDate];
        return;
    }
    
    // check if we are still on track of normal time, otherwise we might have slept or something
    if (tick_time > work_break_duration) {
	// set timers to 0
	micro_pause_t = 0;
	work_break_t = 0;
	micro_pause_taking_t = micro_pause_duration;
	work_break_taking_t = work_break_duration;
	if (s_normal != state) {
	    [self endBreak];
	}
	// and do stuff on next tick
	return;
    }
    
    // just did a whole micropause beyond normal time
    if (tick_time > micro_pause_duration && s_taking_work_break != state) {
	// set micro_pause timers to 0
	micro_pause_t = 0;
	micro_pause_taking_t = micro_pause_duration;
	if (s_normal != state) {
	    [self endBreak];
	}
	// and do stuff on next tick
	return;
    }
    
    // get idle time in seconds
    CFTimeInterval idle_time = CGSSecondsSinceLastInputEvent(kCGAnyInputEventType);
    // CFTimeInterval cgs_idle_time = idle_time;
    // from other people's reverse engineering of this function, on MDD G4s this can return a large positive number when input is in progress
    if (idle_time >= 18446744000.0) {
        idle_time = 0;
    } else if (CGEventSourceSecondsSinceLastEventType != NULL) {
	CGEventType eventTypes[] = { kCGEventLeftMouseDown, kCGEventLeftMouseUp, kCGEventRightMouseDown, kCGEventRightMouseUp, kCGEventMouseMoved, kCGEventLeftMouseDragged, kCGEventRightMouseDragged, kCGEventKeyDown, kCGEventKeyUp, kCGEventFlagsChanged, kCGEventScrollWheel, kCGEventTabletPointer, kCGEventTabletProximity, kCGEventOtherMouseDown, kCGEventOtherMouseUp, kCGEventOtherMouseDragged, kCGEventNull };
        CFTimeInterval event_idle_time;
        idle_time = DBL_MAX;
        for (CGEventType *eventType = eventTypes ; *eventType != kCGEventNull ; eventType++) {
            event_idle_time = CGEventSourceSecondsSinceLastEventType(kCGEventSourceStateCombinedSessionState, *eventType);
            if (event_idle_time < idle_time) idle_time = event_idle_time;
        }
    }
    // NSLog(@"CGEventSource %.2f, CGS %.2f", idle_time, cgs_idle_time);
    
    if (reset_session_timer_after && idle_time > reset_session_timer_after_hours * 3600) {
        [self resetSession:nil];
        return;
    }
    
    // calculate slack, this gives a sort of 3 history filtered idea.
    BOOL slack = (h2 + h1 + h0 > 15);
    
    // if new event comes in history bumps up
    if (h0 >= idle_time || idle_time < sample_interval) {
	h2 = h1;
	h1 = h0;
    }
    h0 = idle_time;
    
    switch (state) {
	case s_normal:
	    // idle_time needs to be at least 0.3 * micro_pause_duration before kicking in
	    // but we cut the user some slack based on previous idle_times
	    if (idle_time <= micro_pause_duration * 0.3 && !slack) {
		micro_pause_t += tick_time;
		work_break_t += tick_time;
                if (idle_time < 1) {
                    session_t += tick_time;
                }
		micro_pause_taking_t = 0;
		if (work_break_taking_t > 0) {
		    work_break_taking_cached_t = work_break_taking_t;
		    work_break_taking_cached_date = date;
		}
		work_break_taking_t = 0;
	    } else if (micro_pause_t > 0) {
		// oke, leaway is over, increase micro_pause_taking_t unless micro_pause is already over
		//micro_pause_t stays put
		work_break_t += tick_time;
		micro_pause_taking_t += tick_time;
		work_break_taking_t = 0;
	    }
	    
	    // if micro_pause_taking_t is above micro_pause_duration, then micro pause is over, 
	    // if still idleing workbreak_taking_t kicks in unless it is already over
	    if (micro_pause_taking_t >= micro_pause_duration && work_break_t > 0) {
		work_break_taking_t += tick_time;
		micro_pause_t = 0;
	    }
	    
	    // if work_break_taking_t is above work_break_duration, then work break is over
	    if (work_break_taking_t >= work_break_duration) {
		micro_pause_t = 0;
		work_break_t = 0;
		// micro_pause_taking_t stays put
		// work_break_taking_t stays put
	    }
	    
	    // if user needs to take a micro pause
	    if (micro_pause_t >= micro_pause_period) {
		// anticipate next workbreak by not issuing this micro_pause ...
		if (work_break_t > work_break_period - (micro_pause_period / 2)) {
		    work_break_t = work_break_period;
		    [self doWorkBreak];
		} else {
		    [self doMicroPause];
		}
	    }
	    
	    // if user needs to take a work break
	    if (work_break_t >= work_break_period) {
		// stop micro_pause stuff
		micro_pause_t = 0;
		micro_pause_taking_t = micro_pause_duration;
		// and display window
		[self doWorkBreak];
	    }
	    break;
	    
	    // taking a micro pause with window
	    case s_taking_micro_pause:
	    // continue updating timers
	    micro_pause_taking_t += tick_time;
	    work_break_t += tick_time;
	    
	    // if we don't break, or interrupt the break, reset it
	    if (idle_time < 1 && !slack) {
		micro_pause_taking_t = 0;
                session_t += tick_time;
	    }
	    
	    // update window
            [self updateBreakWindowDuration:micro_pause_duration progress:micro_pause_taking_t
                                  nextBreak:work_break_period - work_break_t];
	    
	    // check if we done enough
	    if (micro_pause_taking_t > micro_pause_duration) {
		micro_pause_t = 0;
		[self endBreak];
	    }
	    
	    // if workbreak must be run ...
	    if (work_break_t >= work_break_period) {
		// stop micro_pause stuff
		micro_pause_t = 0;
		micro_pause_taking_t = micro_pause_duration;
		// and display window
		[self doWorkBreak];
	    } else {
                double slip = (micro_pause_duration - micro_pause_taking_t) - (int)(micro_pause_duration - micro_pause_taking_t);
                [self installTimer: slip < 0.1 ? 1 : slip];
            }
	    break;
	    
	    // taking a work break with window
	    case s_taking_work_break:
	    // increase work_break_taking_t
	    if (idle_time >= 2 || work_break_taking_t < 3) {
		work_break_taking_t += tick_time;
	    } else if (idle_time < 1) {
                session_t += tick_time;
            }
	    
	    // draw window
            [self updateBreakWindowDuration:work_break_duration progress:work_break_taking_t
                                  nextBreak:work_break_period + work_break_duration - work_break_taking_t];
	    
	    // and check if we done enough
	    if (work_break_taking_t > work_break_duration) {
		micro_pause_t = 0;
		micro_pause_taking_t = micro_pause_duration;
		work_break_t = 0;
		work_break_taking_t = work_break_duration;
		[self endBreak];
	    } else {
                double slip = (work_break_duration - work_break_taking_t) - (int)(work_break_duration - work_break_taking_t);
                [self installTimer: slip < 0.1 ? 1 : slip];
            }
	    break;
    }
    
    // draw dock image
    if (draw_dock_image) [self drawDockImage];
}

// dock image
- (NSMenu *)applicationDockMenu:(NSApplication *)sender;
{
    [session_time_item setTitle:[self sessionTimeString]];
    return dock_menu;
}

// draw the dock icon
- (void)drawDockImage
{
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
    end = 360 - (360.0 / work_break_period * work_break_t - 90);
    if (end <= 90) end=90.1;
    [p appendBezierPathWithArcWithCenter:NSMakePoint(63.5, 63.5) radius:40 startAngle:90 endAngle:end clockwise:YES];
    [p setLineWidth:22];
    [p stroke];
    
    // draw work break taking
    [taking set];
    [p removeAllPoints];
    end = 360 - (360.0 / work_break_duration * work_break_taking_t - 90);
    if (end <= 90) end=90.1;
    [p appendBezierPathWithArcWithCenter:NSMakePoint(63.5, 63.5) radius:40 startAngle:90 endAngle:end clockwise:YES];
    [p setLineWidth:18];
    [p stroke];
    
    // draw micro pause
    [elapsed set];
    [p removeAllPoints];
    end = 360 - (360.0 / micro_pause_period * micro_pause_t - 90);
    if (end <= 90) end = 90.1;
    [p appendBezierPathWithArcWithCenter:NSMakePoint(63.5, 63.5) radius:17 startAngle:90 endAngle:end clockwise:YES];
    [p setLineWidth:22];
    [p stroke];
    
    // draw micro pause taking
    [taking set];
    [p removeAllPoints];
    end = 360 - (360.0 / micro_pause_duration * micro_pause_taking_t - 90);
    if (end <= 90) end = 90.1;
    [p appendBezierPathWithArcWithCenter:NSMakePoint(63.5, 63.5) radius:17 startAngle:90 endAngle:end clockwise:YES];
    [p setLineWidth:18];
    [p stroke];
    
    // draw session time
    if (draw_dock_badge) {
        static NSImage *badge = nil;
        int session_minutes = (int)session_t / 60;
        if (badge_session_minutes != session_minutes) {
            if (badge != nil) [badge release];
            badge = [[dock_badge badgeOverlayImageForString: [NSString stringWithFormat:@"%d:%02d", session_minutes / 60, session_minutes % 60] insetX: 3 y: 3] retain];
            badge_session_minutes = session_minutes;
        }
        [badge compositeToPoint:NSZeroPoint operation:NSCompositeSourceOver];
    }
    
    [dock_image unlockFocus];
    
    // and set it in the dock check draw_dock_image one last time ...
    if (draw_dock_image_q) [NSApp setApplicationIconImage:dock_image];
}

static ProcessSerialNumber frontmostApp;

// done with micro pause or work break
- (void)endBreak
{
    [main_window orderOut:NULL];
    if (lock_focus && (frontmostApp.highLongOfPSN != 0 || frontmostApp.lowLongOfPSN != 0)) {
	SetFrontProcess(&frontmostApp);
	frontmostApp.highLongOfPSN = kNoProcess;
    }
    state = s_normal;
    // reset time interval to user's choice
    [self installTimer:sample_interval];
}

// bring app to front, saving previous app
- (void)focus
{
    NSDictionary *activeProcessInfo = [[NSWorkspace sharedWorkspace] activeApplication];
    if (lock_focus) {
	frontmostApp.highLongOfPSN = [[activeProcessInfo objectForKey:@"NSApplicationProcessSerialNumberHigh"] longValue];
	frontmostApp.lowLongOfPSN = [[activeProcessInfo objectForKey:@"NSApplicationProcessSerialNumberLow"] longValue];
    }
    [self tick: nil];
    [main_window center];
    [main_window orderFrontRegardless];
}

// display micro_pause window with appropriate widgets and progress bar
- (void)doMicroPause
{
    micro_pause_taking_t = 0;
    [status setStringValue:@"Micro Pause"];
    [progress setMaxValue:micro_pause_duration];
    [progress setDoubleValue:micro_pause_taking_t];
    [progress setWarningValue: 1];
    [progress setCriticalValue: micro_pause_duration];
    [postpone setHidden:YES];
    state = s_taking_micro_pause;
    [self focus];
}

// display work_break window with appropriate widgets and progress bar
- (void)doWorkBreak
{
    work_break_taking_t = 0;
    // incase you were already having an implicit work break and clicked the take work break now button
    // not more then 20 seconds ago we took a natural break longer then 0.2 * normal work break duration 
    if (date - work_break_taking_cached_date < 20 && work_break_taking_cached_t > work_break_duration * 0.2) {
	work_break_taking_t = work_break_taking_cached_t;
    } 
    [status setStringValue:@"Work Break"];
    [progress setMaxValue:work_break_duration / 60];
    [progress setDoubleValue:work_break_taking_t / 60 - 0.5];
    [progress setWarningValue: 0];
    [progress setCriticalValue: 0.4];
    [postpone setHidden:NO];
    state = s_taking_work_break;
    [self focus];
}

- (NSString *)sessionTimeString;
{
    int seconds = lrint(session_t);
    return [NSString stringWithFormat:@"Session: %d:%02d:%02d", seconds / 3600,
	    (seconds / 60) % 60, seconds % 60];
}

- (void)updateBreakWindowDuration:(double)duration progress:(double)progress_t nextBreak:(double)nextBreak;
{
    // progress
    [progress setDoubleValue:duration >= 60 ? (progress_t / 60 - 0.5) : progress_t];
    
    // time left
    int timeLeft = lrint(duration - progress_t);
    [time setStringValue:[NSString stringWithFormat:@"%d:%02d", timeLeft / 60, timeLeft % 60]];
    
    // cumulative typing time in this session (e.g. today)
    [session_time setStringValue:[self sessionTimeString]];
    
    // next break
    int minutes = round(nextBreak / 60.0);
    
    // nice hours, minutes ... 
    if (minutes > 60) {
	[next_break setStringValue:[NSString stringWithFormat:@"next break in %d:%02d hours", 
				    minutes / 60, minutes % 60]];
    } else {
	[next_break setStringValue:[NSString stringWithFormat:@"next break in %d minutes", minutes]];
    }
    
    // if user likes to be interrupted
    if (lock_focus) {
        [NSApp activateIgnoringOtherApps:YES];
        [main_window makeKeyAndOrderFront:self];
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
    NSString *latest_version =
    [NSString stringWithContentsOfURL: [NSURL URLWithString:sLatestVersionURL]];
    
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
	     [NSString stringWithFormat:@"A new version (%@) of AntiRSI is available; would you like to go to its Web site now?", latest_version],
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
- (IBAction)postpone:(id)sender
{
    if (s_taking_work_break == state) {
	micro_pause_t = 0;
	micro_pause_taking_t = 0;
	work_break_taking_t = 0;
	work_break_taking_cached_t = 0;
	work_break_t -= 10*60; // decrease with 10 minutes
	if (work_break_t < 0) work_break_t = 0;
	[self endBreak];
    }
}

- (IBAction)breakNow:(id)sender
{
    [self doWorkBreak];
}

- (IBAction)resetSession:(id)sender;
{
    if (s_normal != state) {
        [self endBreak];
    }
    [self resetTimers];
}

// validate menu items
- (BOOL)validateMenuItem:(NSMenuItem *)anItem
{
    if ([anItem action] == @selector(breakNow:) && state == s_normal)
	return YES;
    
    if ([anItem action] == @selector(postpone:) && state == s_taking_work_break)
	return YES;
    
    if ([anItem action] == @selector(resetSession:))
	return YES;
    
    if ([anItem action] == @selector(gotoWebsite:))
	return YES;
    
    if ([anItem action] == @selector(checkForUpdate:))
	return YES;
    
    return NO;
}

// we are delegate of NSApplication, so we can restore the icon on quit.
- (void)applicationWillTerminate:(NSNotification *)aNotification
{
    // make sure timer doesn't tick once more ...
    draw_dock_image_q = NO;
    [mtimer invalidate];
    [mtimer autorelease];
    mtimer = nil;
    [dock_image release];
    // stupid fix for icon beeing restored ... it is not my fault,
    // the dock or NSImage or setApplicationIconImage seem to be caching or taking
    // snapshot or something ... !
    [NSApp setApplicationIconImage:original_dock_image];
    [NSThread sleepUntilDate:[NSDate dateWithTimeIntervalSinceNow:0.1]];
    [NSApp setApplicationIconImage:original_dock_image];
    
}

@end

