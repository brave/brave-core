/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "src/components/remote_cocoa/app_shim/native_widget_ns_window_bridge.mm"

namespace remote_cocoa {

void NativeWidgetNSWindowBridge::SetWindowTitleVisibility(bool visible) {
  NSUInteger styleMask = [window_ styleMask];
  if (visible)
    styleMask |= NSWindowStyleMaskTitled;
  else
    styleMask &= ~NSWindowStyleMaskTitled;

  [window_ setStyleMask:styleMask];

  ResetWindowControlsPosition();

  // Sometimes title is not visible until window is resized. In order to avoid
  // this, reset title to force it to be visible.
  if (visible) {
    NSString* title = window_.get().title;
    window_.get().title = @"";
    window_.get().title = title;
  }
}

void NativeWidgetNSWindowBridge::ResetWindowControlsPosition() {
  // Call undocumented method of NSThemeFrame in order to reset window controls'
  // position.
  NSView* frameView = window_.get().contentView.superview;
  DCHECK([frameView isKindOfClass:[NSThemeFrame class]]);
  [frameView performSelector:@selector(_resetTitleBarButtons)];
}

}  // namespace remote_cocoa
