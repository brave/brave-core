/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_VIEWS_TABS_TAB_GROUP_STYLE_VIEWS_H_
#define BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_VIEWS_TABS_TAB_GROUP_STYLE_VIEWS_H_

#define TabGroupStyleViews TabGroupStyleViews_ChromiumImpl
#include <chrome/browser/ui/views/tabs/tab_group_style_views.h>  // IWYU pragma: export
#undef TabGroupStyleViews

class TabGroupStyleViews : public TabGroupStyleViews_ChromiumImpl {
 public:
  using TabGroupStyleViews_ChromiumImpl::TabGroupStyleViews_ChromiumImpl;

  static constexpr int kStrokeThicknessForVerticalTabs = 4;

  bool TabGroupUnderlineShouldBeHidden() const override;

  bool TabGroupUnderlineShouldBeHidden(
      const views::View* leading_view,
      const views::View* trailing_view) const override;

  SkPath GetUnderlinePath(gfx::Rect local_bounds) const override;

 private:
  bool ShouldShowBraveVerticalTabs() const;
};

#endif  // BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_VIEWS_TABS_TAB_GROUP_STYLE_VIEWS_H_
