/*
 * author: Onne Gorter <o.gorter@gmail.com>
 * package: antirsi-macosx
 * license: GPL
 */

#import <Cocoa/Cocoa.h>

@interface AntiRSIView : NSView
{
	NSImage* _image;
}

- (void)setImage:(NSImage *)image;

@end
