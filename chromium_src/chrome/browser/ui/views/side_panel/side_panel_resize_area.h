// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_VIEWS_SIDE_PANEL_SIDE_PANEL_RESIZE_AREA_H_
#define BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_VIEWS_SIDE_PANEL_SIDE_PANEL_RESIZE_AREA_H_

#define UpdateHandleVisibility(...)    \
  UpdateHandleVisibility(__VA_ARGS__); \
  friend class BraveSidePanelResizeArea

#include <chrome/browser/ui/views/side_panel/side_panel_resize_area.h>  // IWYU pragma: export

#undef UpdateHandleVisibility

#endif  // BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_VIEWS_SIDE_PANEL_SIDE_PANEL_RESIZE_AREA_H_
