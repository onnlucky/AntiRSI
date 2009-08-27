/*
 * author: Onne Gorter <o.gorter@gmail.com>
 * package: antirsi-macosx
 * license: GPL
 */

#import "AntiRSIView.h"

@implementation AntiRSIView

- (void)drawRect:(NSRect)rect {
    NSColor *bgColor = [NSColor colorWithCalibratedWhite:0.0 alpha:0.95];
    
    NSRect bgRect = [self frame];
    int minX = NSMinX(bgRect);
    int midX = NSMidX(bgRect);
    int maxX = NSMaxX(bgRect);
    int minY = NSMinY(bgRect);
    int midY = NSMidY(bgRect);
    int maxY = NSMaxY(bgRect);
    
    // correct value to duplicate Panther's App Switcher
    float radius = 25.0;
    
    NSBezierPath *bgPath = [NSBezierPath bezierPath];
    
    /* XXX from Casey Marshall's version; does it help with the hole-in-window problem? */
    [[NSColor clearColor] set];
    NSRectFill(bgRect);
    /* XXX end */
    
    // Bottom edge and bottom-right curve
    [bgPath moveToPoint:NSMakePoint(midX, minY)];
    [bgPath appendBezierPathWithArcFromPoint:NSMakePoint(maxX, minY) 
                                     toPoint:NSMakePoint(maxX, midY) 
                                      radius:radius];
    
    // Right edge and top-right curve
    [bgPath appendBezierPathWithArcFromPoint:NSMakePoint(maxX, maxY) 
                                     toPoint:NSMakePoint(midX, maxY) 
                                      radius:radius];
    
    // Top edge and top-left curve
    [bgPath appendBezierPathWithArcFromPoint:NSMakePoint(minX, maxY) 
                                     toPoint:NSMakePoint(minX, midY) 
                                      radius:radius];
    
    // Left edge and bottom-left curve
    [bgPath appendBezierPathWithArcFromPoint:bgRect.origin 
                                     toPoint:NSMakePoint(midX, minY) 
                                      radius:radius];
    [bgPath closePath];
    
    [bgColor set];
    [bgPath fill];
}

@end


@implementation AntiRSIButton

// prevents AntiRSI activation on mouse down
- (BOOL)shouldDelayWindowOrderingForEvent:(NSEvent *)e {
    return YES;
}

- (void)mouseDown:(NSEvent *)e {
    [NSApp preventWindowOrdering];
    [super mouseDown: e];
}

@end
