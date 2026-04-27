// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#ifndef BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_VIEWS_SIDE_PANEL_SIDE_PANEL_H_
#define BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_VIEWS_SIDE_PANEL_SIDE_PANEL_H_

#include <memory>

#include "brave/browser/ui/sidebar/buildflags/buildflags.h"

#if BUILDFLAG(ENABLE_SIDEBAR_V2)
// V2: use upstream's SidePanel class directly.
// Angle brackets are required here to resolve to the upstream file rather than
// this chromium_src override (which would cause infinite recursion).

// Need SetResizeArea() to replace resize area to customize its position, and
// to support toggling the border at runtime (disabling it makes content fill
// the panel).
#define GetContentParentView(...)                                        \
  GetContentParentView(__VA_ARGS__);                                     \
  void SetResizeArea(std::unique_ptr<views::View> resize_area);          \
  void SetRoundedBorderEnabled(bool enabled);                            \
  void UpdateBorder();                                                   \
  void VisibilityChanged(View* starting_from, bool is_visible) override; \
  void RemoveHeaderView_UnUsed()

#define did_resize_    \
  did_resize_ = false; \
  bool rounded_border_enabled_

#include <chrome/browser/ui/views/side_panel/side_panel.h>  // IWYU pragma: export

#undef did_resize_
#undef GetContentParentView

#else
// V1: use Brave's custom SidePanel replacement, which also prevents
// `chrome/browser/ui/views/side_panel/side_panel.cc` from being built.
#include "brave/browser/ui/views/side_panel/side_panel.h"  // IWYU pragma: export
#endif

#endif  // BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_VIEWS_SIDE_PANEL_SIDE_PANEL_H_
