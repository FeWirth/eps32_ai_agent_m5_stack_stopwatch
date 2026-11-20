#import <AppKit/AppKit.h>
#include "avatar.hpp"

class Screen final : public hg::Display {
 public:
  hg::DisplayInfo info() const override {
    hg::DisplayInfo result;
    result.width = result.height = alter_ego::size;
    result.swap_bytes = result.round = true;
    return result;
  }
  uint16_t* framebuffer() override { return pixels; }
  void flush(uint16_t, uint16_t) override {}
  uint16_t pixels[alter_ego::size * alter_ego::size]{};
};

static uint32_t milliseconds() {
  const auto uptime = static_cast<uint64_t>(NSProcessInfo.processInfo.systemUptime * 1000);
  return static_cast<uint32_t>(uptime);
}

@interface AvatarView : NSView
- (NSBitmapImageRep*)renderBitmap;
@end

@implementation AvatarView {
  Screen _screen;
  alter_ego::Avatar _avatar;
}

- (NSBitmapImageRep*)renderBitmap {
  constexpr int size = alter_ego::size;
  _avatar.draw(_screen, milliseconds());
  auto* bitmap = [[NSBitmapImageRep alloc]
      initWithBitmapDataPlanes:nullptr pixelsWide:size pixelsHigh:size bitsPerSample:8
      samplesPerPixel:4 hasAlpha:YES isPlanar:NO colorSpaceName:NSDeviceRGBColorSpace
      bytesPerRow:size * 4 bitsPerPixel:32];
  auto* dest = bitmap.bitmapData;
  const auto* wire = reinterpret_cast<const uint8_t*>(_screen.pixels);
  for (int i = 0; i < size * size; ++i) {
    const uint16_t word = (wire[2 * i] << 8) | wire[2 * i + 1];
    dest[4 * i] = ((word >> 11) & 31) * 255 / 31;
    dest[4 * i + 1] = ((word >> 5) & 63) * 255 / 63;
    dest[4 * i + 2] = (word & 31) * 255 / 31;
    dest[4 * i + 3] = 255;
  }
  return bitmap;
}

- (void)drawRect:(NSRect)rect {
  (void)rect;
  [[self renderBitmap] drawInRect:self.bounds];
}

- (void)input:(NSEvent*)event down:(BOOL)down {
  const auto point = [self convertPoint:event.locationInWindow fromView:nil];
  _avatar.touch(down, int(point.x * alter_ego::size / self.bounds.size.width),
                int((self.bounds.size.height - point.y) * alter_ego::size / self.bounds.size.height),
                milliseconds());
  self.needsDisplay = YES;
}
- (void)mouseDown:(NSEvent*)event { [self input:event down:YES]; }
- (void)mouseDragged:(NSEvent*)event { [self input:event down:YES]; }
- (void)mouseUp:(NSEvent*)event { [self input:event down:NO]; }
@end

@interface PreviewDelegate : NSObject <NSApplicationDelegate>
@end
@implementation PreviewDelegate
- (BOOL)applicationShouldTerminateAfterLastWindowClosed:(NSApplication*)app {
  (void)app;
  return YES;
}
@end

int main(int argc, const char** argv) {
  @autoreleasepool {
    const auto bounds = NSMakeRect(0, 0, alter_ego::size, alter_ego::size);
    auto* view = [[AvatarView alloc] initWithFrame:bounds];
    if (argc == 2) {
      auto* png = [[view renderBitmap] representationUsingType:NSBitmapImageFileTypePNG properties:@{}];
      NSError* error = nil;
      if ([png writeToFile:@(argv[1]) options:0 error:&error]) return 0;
      NSLog(@"PNG export failed: %@", error);
      return 1;
    }

    auto* app = NSApplication.sharedApplication;
    auto* delegate = [PreviewDelegate new];
    app.delegate = delegate;
    [app setActivationPolicy:NSApplicationActivationPolicyRegular];
    auto* window = [[NSWindow alloc] initWithContentRect:bounds
        styleMask:NSWindowStyleMaskTitled | NSWindowStyleMaskClosable
        backing:NSBackingStoreBuffered defer:NO];
    window.title = @"Himmelsgravur · Alter Ego";
    window.contentView = view;
    [window center];
    [window makeKeyAndOrderFront:nil];
    [app activateIgnoringOtherApps:YES];
    auto* timer = [NSTimer timerWithTimeInterval:1.0 / 24 repeats:YES block:^(NSTimer*) {
      view.needsDisplay = YES;
    }];
    [NSRunLoop.mainRunLoop addTimer:timer forMode:NSRunLoopCommonModes];
    [app run];
    [timer invalidate];
    return 0;
  }
}
