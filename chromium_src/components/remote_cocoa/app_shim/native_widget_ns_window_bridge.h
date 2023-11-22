/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_COMPONENTS_REMOTE_COCOA_APP_SHIM_NATIVE_WIDGET_NS_WINDOW_BRIDGE_H_
#define BRAVE_CHROMIUM_SRC_COMPONENTS_REMOTE_COCOA_APP_SHIM_NATIVE_WIDGET_NS_WINDOW_BRIDGE_H_

// Override a method from our NativeWidgetNSWindow mojo extension
#define OnSizeChanged                                  \
  SetWindowTitleVisibility(bool visible) override;     \
  void ResetWindowControlsPosition() override;         \
  void UpdateWindowTitleColor(SkColor color) override; \
  void OnSizeChanged

#include "src/components/remote_cocoa/app_shim/native_widget_ns_window_bridge.h"  // IWYU pragma: export

#undef OnSizeChanged

#endif  // BRAVE_CHROMIUM_SRC_COMPONENTS_REMOTE_COCOA_APP_SHIM_NATIVE_WIDGET_NS_WINDOW_BRIDGE_H_
