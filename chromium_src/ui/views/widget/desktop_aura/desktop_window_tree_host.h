/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_UI_VIEWS_WIDGET_DESKTOP_AURA_DESKTOP_WINDOW_TREE_HOST_H_
#define BRAVE_CHROMIUM_SRC_UI_VIEWS_WIDGET_DESKTOP_AURA_DESKTOP_WINDOW_TREE_HOST_H_

#define GetSingletonDesktopNativeCursorManager() \
  GetSingletonDesktopNativeCursorManager();      \
  virtual SkColor GetBackgroundColor(SkColor requested_color) const

#include "src/ui/views/widget/desktop_aura/desktop_window_tree_host.h"  // IWYU pragma: export

#undef GetSingletonDesktopNativeCursorManager

#endif  // BRAVE_CHROMIUM_SRC_UI_VIEWS_WIDGET_DESKTOP_AURA_DESKTOP_WINDOW_TREE_HOST_H_
