/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_UI_VIEWS_COCOA_NATIVE_WIDGET_MAC_NS_WINDOW_HOST_H_
#define BRAVE_CHROMIUM_SRC_UI_VIEWS_COCOA_NATIVE_WIDGET_MAC_NS_WINDOW_HOST_H_

#define OnWidgetInitDone                  \
  SetWindowTitleVisibility(bool visible); \
  void OnWidgetInitDone

#include "src/ui/views/cocoa/native_widget_mac_ns_window_host.h"  // IWYU pragma: export

#undef OnWidgetInitDone

#endif  // BRAVE_CHROMIUM_SRC_UI_VIEWS_COCOA_NATIVE_WIDGET_MAC_NS_WINDOW_HOST_H_
