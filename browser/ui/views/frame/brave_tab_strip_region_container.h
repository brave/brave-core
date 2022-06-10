/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_VIEWS_FRAME_BRAVE_TAB_STRIP_REGION_CONTAINER_H_
#define BRAVE_BROWSER_UI_VIEWS_FRAME_BRAVE_TAB_STRIP_REGION_CONTAINER_H_

#include "chrome/browser/ui/views/frame/tab_strip_region_view.h"

namespace views {
class ScrollView;
}

// Wraps TabStripRegion and show it vertically.
class BraveTabStripRegionContainer : public views::View {
 public:
  METADATA_HEADER(BraveTabStripRegionContainer);

  enum class State { kCollapsed, kExpanded };

  explicit BraveTabStripRegionContainer(TabStripRegionView* region_view);
  ~BraveTabStripRegionContainer() override;

  // views::View:
  gfx::Size CalculatePreferredSize() const override;
  void Layout() override;
  void OnThemeChanged() override;

 private:
  void SetState(State state);

  void UpdateLayout();

  void UpdateNewTabButtonVisibility();

  raw_ptr<views::View> original_parent_of_region_view_ = nullptr;
  raw_ptr<TabStripRegionView> region_view_ = nullptr;

  // Contains TabStripRegion.
  raw_ptr<views::ScrollView> scroll_view_ = nullptr;
  raw_ptr<views::View> scroll_view_header_ = nullptr;

  // New tab button created for vertical tabs
  raw_ptr<NewTabButton> new_tab_button_ = nullptr;

  State state_ = State::kExpanded;
};

#endif  // BRAVE_BROWSER_UI_VIEWS_FRAME_BRAVE_TAB_STRIP_REGION_CONTAINER_H_
