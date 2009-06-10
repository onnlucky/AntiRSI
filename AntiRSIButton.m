//
//  AntiRSIButton.m
//  AntiRSI
//
//  Created by Nicholas Riley on 12/7/07.
//  Copyright 2007 Nicholas Riley. All rights reserved.
//

#import "AntiRSIButton.h"


@implementation AntiRSIButton

// prevents AntiRSI activation on mouse down
- (BOOL)shouldDelayWindowOrderingForEvent:(NSEvent *)theEvent
{
    return YES;
}

- (void)mouseDown:(NSEvent *)theEvent;
{
    [NSApp preventWindowOrdering];
    [super mouseDown: theEvent];
}

@end
