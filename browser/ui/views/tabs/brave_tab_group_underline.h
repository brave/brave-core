/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_VIEWS_TABS_BRAVE_TAB_GROUP_UNDERLINE_H_
#define BRAVE_BROWSER_UI_VIEWS_TABS_BRAVE_TAB_GROUP_UNDERLINE_H_

#include "chrome/browser/ui/views/tabs/tab_group_underline.h"

// In vertical tabs, TabGroupUnderline is not actually "underline'. It's an
// enclosing rounded rect for views in the group.
class BraveTabGroupUnderline : public TabGroupUnderline {
 public:
  BraveTabGroupUnderline(TabGroupViews* tab_group_views,
                         const tab_groups::TabGroupId& group);
  ~BraveTabGroupUnderline() override;

  // TabGroupUnderline:
  void UpdateBounds(views::View* leading_view,
                    views::View* trailing_view) override;
  gfx::Insets GetInsetsForUnderline(views::View* sibling_view) const override;
  SkPath GetPath() const override;
  void OnPaint(gfx::Canvas* canvas) override;
};

#endif  // BRAVE_BROWSER_UI_VIEWS_TABS_BRAVE_TAB_GROUP_UNDERLINE_H_
