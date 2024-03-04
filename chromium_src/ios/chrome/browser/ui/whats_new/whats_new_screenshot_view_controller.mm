// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

// This UI code isn't used in iOS at all anyways but causes a failure when
// running unit tests due to a static initializer that references UIKit too
// early.
//
// This chromium_src override can be removed in CR124+ as that static
// initializer was removed
// (https://chromium-review.googlesource.com/c/chromium/src/+/5315216)

#import "ios/chrome/browser/ui/whats_new/whats_new_screenshot_view_controller.h"

@implementation WhatsNewScreenshotViewController
- (instancetype)initWithWhatsNewItem:(WhatsNewItem*)item {
  return [super initWithNibName:nil bundle:nil];
}
@end
