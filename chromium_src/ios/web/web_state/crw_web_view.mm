// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#import <UIKit/UIKit.h>

#include <ios/web/web_state/crw_web_view.mm>

@implementation CRWWebView (Brave)

- (UIEdgeInsets)safeAreaInsets {
  // In iOS 26+ we are using the new `obscuredContentInsets` API to fix the
  // pages viewport and allow us to apply decorative blur effects behind
  // toolbars. Using this API though only seems to only work correctly when the
  // `safeAreaInsets` are set to 0 because we have to add the safe area already
  // to the obscuredContentInsets for it to adjust the viewport correctly.
  // WKWebView will still report the safe area to the page despite the viewport
  // CSS metrics being correctly reported and you end up with pages like YouTube
  // having extra whitespace added to the top & bottom of the page.
  //
  // Chromium may support this properly through CRWViewportAdjustment in the
  // future but for now we have to use the WKWebView API directly.
  if (@available(iOS 26.0, *)) {
    return UIEdgeInsetsZero;
  }
  return [super safeAreaInsets];
}

@end
