// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#ifndef BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_VIEWS_SIDE_PANEL_SIDE_PANEL_H_
#define BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_VIEWS_SIDE_PANEL_SIDE_PANEL_H_

#include <memory>

// Need SetResizeArea() to replace resize area to customize its position, and
// to support toggling the border at runtime (disabling it makes content fill
// the panel).

#define GetContentParentView(...)                                     \
  GetContentParentView(__VA_ARGS__);                                  \
  void SetResizeArea(std::unique_ptr<views::View> resize_area);       \
  void SetRoundedBorderEnabled(bool enabled);                         \
  void UpdateBorder();                                                \
  void AddHeaderView_ChromiumImpl(std::unique_ptr<views::View> view); \
  void RemoveHeaderView_ChromiumImpl()

#define did_resize_    \
  did_resize_ = false; \
  bool rounded_border_enabled_

#include <chrome/browser/ui/views/side_panel/side_panel.h>  // IWYU pragma: export

#undef did_resize_
#undef GetContentParentView

#endif  // BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_VIEWS_SIDE_PANEL_SIDE_PANEL_H_
