// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_VIEWS_SIDE_PANEL_SIDE_PANEL_RESIZE_AREA_H_
#define BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_VIEWS_SIDE_PANEL_SIDE_PANEL_RESIZE_AREA_H_

#include "brave/browser/ui/sidebar/buildflags/buildflags.h"

#if BUILDFLAG(ENABLE_SIDEBAR_V2)
// V2: upstream's SidePanel uses SidePanelResizeArea directly.

#define UpdateHandleVisibility(...)    \
  UpdateHandleVisibility(__VA_ARGS__); \
  friend class BraveSidePanelResizeArea

#include <chrome/browser/ui/views/side_panel/side_panel_resize_area.h>  // IWYU pragma: export

#undef UpdateHandleVisibility

#else
// V1: excluded — Brave uses SidePanelResizeWidget instead.
#endif

#endif  // BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_VIEWS_SIDE_PANEL_SIDE_PANEL_RESIZE_AREA_H_
