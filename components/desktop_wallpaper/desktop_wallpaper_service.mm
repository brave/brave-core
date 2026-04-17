#include "desktop_wallpaper_service.h"

#include <AppKit/AppKit.h>
#import <Appkit/AppKit.h>
#include <CoreGraphics/CGDirectDisplay.h>
#import <Foundation/Foundation.h>

#include <cstdint>
#include <vector>

#include "brave/browser/ui/webui/desktop_wallpaper/desktop_wallpaper.mojom.h"

namespace desktop_wallpaper {
desktop_wallpaper::mojom::WallpaperStatus DesktopWallpaper::SetWallpaper(
    base::FilePath path,
    std::vector<desktop_wallpaper::mojom::DisplayInfosPtr> displays,
    Scaling scaling) {
  @autoreleasepool {
    NSURL* url = [NSURL fileURLWithPath:@(path.value().c_str())];
    NSArray<NSScreen*>* screens = [NSScreen screens];

    NSInteger image_scaling;
    switch (scaling) {
      case Scaling::kFitToScreen:
        image_scaling = NSImageScaleProportionallyUpOrDown;
        break;
      case Scaling::kFillScreen:
        image_scaling = NSImageScaleProportionallyUpOrDown;
        break;
      case Scaling::kStretchToFill:
        image_scaling = NSImageScaleAxesIndependently;
        break;
      case Scaling::kCenter:
        image_scaling = NSImageScaleNone;
        break;
    }

    NSDictionary* options = @{
      NSWorkspaceDesktopImageScalingKey : @(image_scaling),
      NSWorkspaceDesktopImageAllowClippingKey :
          @(scaling == Scaling::kFillScreen)
    };

    for (NSScreen* screen in screens) {
      CGDirectDisplayID screen_id =
          [[screen deviceDescription][@"NSScreenNumber"] unsignedIntValue];

      for (auto& display : displays) {
        NSError* err = nil;

        if (std::stoll(display->id) == screen_id) {
           BOOL result = [[NSWorkspace sharedWorkspace] setDesktopImageURL:url
                                                           forScreen:screen
                                                             options:options
                                                               error:&err];

          if (!result || err) {
            return desktop_wallpaper::mojom::WallpaperStatus::failure;
          }
        }
      }
    }

    return desktop_wallpaper::mojom::WallpaperStatus::success;
  }
}
}  // namespace desktop_wallpaper
