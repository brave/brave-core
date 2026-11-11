/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#import <Foundation/Foundation.h>

namespace {

// Dock tile badge for in-progress downloads, or nil. Percentage only when
// there is a single determinate download; otherwise the count.
NSString* DockBadgeLabel(int downloads, BOOL indeterminate, float progress) {
  if (downloads == 0) {
    return nil;
  }

  NSNumberFormatter* formatter = [[NSNumberFormatter alloc] init];
  if (downloads > 1 || indeterminate) {
    return [formatter stringFromNumber:@(downloads)];
  }

  formatter.numberStyle = NSNumberFormatterPercentStyle;
  formatter.maximumFractionDigits = 0;
  return [formatter stringFromNumber:@(progress)];
}

}  // namespace

#include <chrome/browser/ui/cocoa/dock_icon.mm>
