/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/frame/brave_browser_view_mac.h"

#import <AppKit/AppKit.h>

#include <algorithm>

namespace brave {

void SetTrafficLightsAlpha(gfx::NativeWindow window, double alpha) {
  NSWindow* ns_window = window.GetNativeNSWindow();
  if (!ns_window) {
    return;
  }

  const CGFloat clamped = std::clamp(alpha, 0.0, 1.0);
  const BOOL enabled = clamped > 0.0;

  constexpr NSWindowButton kButtons[] = {
      NSWindowCloseButton,
      NSWindowMiniaturizeButton,
      NSWindowZoomButton,
  };

  for (NSWindowButton button_type : kButtons) {
    if (NSButton* button = [ns_window standardWindowButton:button_type]) {
      button.alphaValue = clamped;
      button.enabled = enabled;
    }
  }
}

}  // namespace brave
