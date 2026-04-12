#include "desktop_wallpaper_service.h"

#import <Appkit/AppKit.h>
#import <Foundation/Foundation.h>

namespace desktop_wallpaper {
void DesktopWallpaper::SetWallpaper(base::FilePath path) {
  @autoreleasepool {
    NSURL* url = [NSURL fileURLWithPath:@(path.value().c_str())];
    NSError* err = nil;

    [[NSWorkspace sharedWorkspace] setDesktopImageURL:url
                                            forScreen:[NSScreen mainScreen]
                                              options:{}
                                                error:&err];
  }
}
}  // namespace desktop_wallpaper
