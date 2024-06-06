/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_VIEWS_TABS_TAB_GROUP_STYLE_H_
#define BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_VIEWS_TABS_TAB_GROUP_STYLE_H_

#define TabGroupStyle TabGroupStyle_ChromiumImpl
#include "src/chrome/browser/ui/views/tabs/tab_group_style.h"  // IWYU pragma: export
#undef TabGroupStyle

class TabGroupStyle : public TabGroupStyle_ChromiumImpl {
 public:
  using TabGroupStyle_ChromiumImpl::TabGroupStyle_ChromiumImpl;

  static const int kStrokeThicknessForVerticalTabs;

  bool TabGroupUnderlineShouldBeHidden() const override;

  bool TabGroupUnderlineShouldBeHidden(
      const views::View* leading_view,
      const views::View* trailing_view) const override;

  SkPath GetUnderlinePath(gfx::Rect local_bounds) const override;

  gfx::Insets GetInsetsForHeaderChip(bool should_show_sync_icon) const override;

  gfx::Point GetTitleChipOffset(std::optional<int> text_height) const override;

  int GetChipCornerRadius() const override;

  float GetEmptyChipSize() const override;

 private:
  bool ShouldShowVerticalTabs() const;
};

#endif  // BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_VIEWS_TABS_TAB_GROUP_STYLE_H_
