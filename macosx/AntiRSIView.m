/*
 * author: Onne Gorter <o.gorter@gmail.com>
 * package: antirsi-macosx
 * license: GPL
 */

#import "AntiRSIView.h"

@implementation AntiRSIView

- (id)initWithFrame:(NSRect)frameRect
{
	if ((self = [super initWithFrame:frameRect]) != nil) {
		// Add initialization code here
	}
	return self;
}

- (void)drawRect:(NSRect)rect
{
	[[NSColor clearColor] set];
    NSRectFill([self frame]);
	[_image compositeToPoint:NSZeroPoint operation:NSCompositeSourceOver];
	
}

- (void)setImage:(NSImage *)image;
{
	_image = image;
	[self setNeedsDisplay:YES];
}
@end
