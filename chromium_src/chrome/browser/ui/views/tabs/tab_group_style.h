/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_VIEWS_TABS_TAB_GROUP_STYLE_H_
#define BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_VIEWS_TABS_TAB_GROUP_STYLE_H_

#define TabGroupStyle TabGroupStyle_ChromiumImpl
#define ChromeRefresh2023TabGroupStyle \
  ChromeRefresh2023TabGroupStyle_ChromiumImpl

#include "src/chrome/browser/ui/views/tabs/tab_group_style.h"  // IWYU pragma: export
#undef ChromeRefresh2023TabGroupStyle
#undef TabGroupStyle

class TabGroupStyle : public TabGroupStyle_ChromiumImpl {
 public:
  using TabGroupStyle_ChromiumImpl::TabGroupStyle_ChromiumImpl;

  static const int kStrokeThicknessForVerticalTabs;

  SkPath GetUnderlinePath(gfx::Rect local_bounds) const override;

  gfx::Insets GetInsetsForHeaderChip(bool should_show_sync_icon) const override;

  gfx::Point GetTitleChipOffset(std::optional<int> text_height) const override;

  int GetChipCornerRadius() const override;

  float GetEmptyChipSize() const override;

 private:
  bool ShouldShowVerticalTabs() const;
};

// Clobber ChromeRefresh2023TabGroupStyle for now because of how
// chrome/browser/ui/views/tabs/tab_group_views.cc wants to instantiate the
// style:
// style_ = features::IsChromeRefresh2023()
//   ? std::make_unique<const ChromeRefresh2023TabGroupStyle>(*this)
//   : std::make_unique<const TabGroupStyle>(*this);
// and ChromeRefresh2023TabGroupStyle is no longer inheriting from TabGroupStyle
// due to the above redefine.
class ChromeRefresh2023TabGroupStyle : public TabGroupStyle {
 public:
  using TabGroupStyle::TabGroupStyle;

  // A necessary stub to go back to the original class, as this static gets used
  // by `TabGroupUnderline::GetStrokeInset`.
  static int GetTabGroupOverlapAdjustment();
};

#endif  // BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_VIEWS_TABS_TAB_GROUP_STYLE_H_
