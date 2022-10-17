/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_VIEWS_FRAME_VERTICAL_TAB_STRIP_REGION_VIEW_H_
#define BRAVE_BROWSER_UI_VIEWS_FRAME_VERTICAL_TAB_STRIP_REGION_VIEW_H_

#include "chrome/browser/ui/views/frame/tab_strip_region_view.h"
#include "components/prefs/pref_member.h"

namespace views {
class ScrollView;
}

class Browser;

// Wraps TabStripRegion and show it vertically.
class VerticalTabStripRegionView : public views::View {
 public:
  METADATA_HEADER(VerticalTabStripRegionView);

  enum class State { kCollapsed, kExpanded };

  VerticalTabStripRegionView(Browser* browser, TabStripRegionView* region_view);
  ~VerticalTabStripRegionView() override;

  State state() const { return state_; }

  const TabStrip* tab_strip() const { return region_view_->tab_strip_; }
  TabStrip* tab_strip() { return region_view_->tab_strip_; }

  const Browser* browser() const { return browser_; }

  // views::View:
  gfx::Size CalculatePreferredSize() const override;
  void Layout() override;
  void OnThemeChanged() override;

 private:
  bool IsTabFullscreen() const;

  void SetState(State state);

  void UpdateLayout();

  void UpdateNewTabButtonVisibility();
  void UpdateTabSearchButtonVisibility();

  void OnCollapsedPrefChanged();

  raw_ptr<Browser> browser_ = nullptr;

  raw_ptr<views::View> original_parent_of_region_view_ = nullptr;
  raw_ptr<TabStripRegionView> region_view_ = nullptr;

  // Contains TabStripRegion.
  raw_ptr<views::ScrollView> scroll_view_ = nullptr;
  raw_ptr<views::View> scroll_contents_view_ = nullptr;
  raw_ptr<views::View> scroll_view_header_ = nullptr;

  // New tab button created for vertical tabs
  raw_ptr<NewTabButton> new_tab_button_ = nullptr;

  State state_ = State::kExpanded;

  BooleanPrefMember collapsed_pref_;
};

#endif  // BRAVE_BROWSER_UI_VIEWS_FRAME_VERTICAL_TAB_STRIP_REGION_VIEW_H_
