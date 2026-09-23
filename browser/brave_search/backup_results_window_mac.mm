// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/brave_search/backup_results_window_mac.h"

#import <AppKit/AppKit.h>

#include <memory>

#include "content/public/browser/web_contents.h"
#include "ui/gfx/geometry/rect.h"
#include "ui/gfx/geometry/size.h"
#include "ui/gfx/mac/coordinate_conversion.h"

// An NSWindow that can never be placed on screen.
@interface BraveBackupResultsWindow : NSWindow
@end

@implementation BraveBackupResultsWindow

- (void)orderWindow:(NSWindowOrderingMode)orderingMode
         relativeTo:(NSInteger)otherWindowNumber {
  // We don't want to allow ordering in, as that would make the window visible.
  // Ordering out is still honored.
  if (orderingMode == NSWindowOut) {
    [super orderWindow:orderingMode relativeTo:otherWindowNumber];
  }
}

- (void)setIsVisible:(BOOL)flag {
  // Intentionally ignored.
}

- (BOOL)canBecomeKeyWindow {
  return NO;
}

- (BOOL)canBecomeMainWindow {
  return NO;
}

@end

namespace brave_search {

namespace {

class BackupResultsWindowMacImpl : public BackupResultsWindowMac {
 public:
  BackupResultsWindowMacImpl(NSView* contents_view,
                             const gfx::Rect& window_bounds,
                             const gfx::Size& view_size)
      : contents_view_(contents_view) {
    // Borderless so the frame and content rect match
    window_ = [[BraveBackupResultsWindow alloc]
        initWithContentRect:gfx::ScreenRectToNSRect(window_bounds)
                  styleMask:NSWindowStyleMaskBorderless
                    backing:NSBackingStoreBuffered
                      defer:YES];
    window_.releasedWhenClosed = NO;
    window_.excludedFromWindowsMenu = YES;

    // Set the view size and align with the bottom left of the window.
    contents_view_.frame =
        NSMakeRect(0, 0, view_size.width(), view_size.height());

    // Frame first, so the resulting `-viewDidMoveToWindow` reports final
    // bounds.
    [window_.contentView addSubview:contents_view_];
  }

  ~BackupResultsWindowMacImpl() override {
    // `RenderWidgetHostNSViewBridge` removes its view from a delayed selector,
    // after the WebContents is gone. Detach now so neither outlives the window.
    [contents_view_ removeFromSuperview];
    contents_view_ = nil;
    window_ = nil;
  }

 private:
  NSView* __strong contents_view_;
  BraveBackupResultsWindow* __strong window_;
};

}  // namespace

// static
std::unique_ptr<BackupResultsWindowMac> BackupResultsWindowMac::Create(
    content::WebContents* web_contents,
    const gfx::Rect& window_bounds,
    const gfx::Size& view_size) {
  NSView* contents_view = web_contents->GetNativeView().GetNativeNSView();
  if (!contents_view) {
    return nullptr;
  }
  return std::make_unique<BackupResultsWindowMacImpl>(contents_view,
                                                      window_bounds, view_size);
}

BackupResultsWindowMac::BackupResultsWindowMac() = default;

BackupResultsWindowMac::~BackupResultsWindowMac() = default;

}  // namespace brave_search
