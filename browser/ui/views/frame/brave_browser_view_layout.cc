// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#include "brave/browser/ui/views/frame/brave_browser_view_layout.h"

#include <vector>

#include "brave/browser/ui/views/tabs/features.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/views/bookmarks/bookmark_bar_view.h"
#include "chrome/browser/ui/views/frame/browser_view.h"
#include "chrome/browser/ui/views/frame/browser_view_layout_delegate.h"
#include "chrome/browser/ui/views/infobars/infobar_container_view.h"
#include "components/bookmarks/common/bookmark_pref_names.h"

BraveBrowserViewLayout::~BraveBrowserViewLayout() = default;

void BraveBrowserViewLayout::Layout(views::View* host) {
  BrowserViewLayout::Layout(host);
  if (!vertical_tab_strip_host_.get())
    return;

  DCHECK(base::FeatureList::IsEnabled(tabs::features::kBraveVerticalTabs))
      << "vertical_tab_strip_host_ should be set only when this flag is on";

  if (!tabs::features::ShouldShowVerticalTabs(browser_view_->browser())) {
    vertical_tab_strip_host_->SetBorder(nullptr);
    vertical_tab_strip_host_->SetBoundsRect({});
    return;
  }

  std::vector<views::View*> views_next_to_vertical_tabs;
  if (ShouldPushBookmarkBarForVerticalTabs())
    views_next_to_vertical_tabs.push_back(bookmark_bar_);
  if (infobar_container_->GetVisible())
    views_next_to_vertical_tabs.push_back(infobar_container_);
  views_next_to_vertical_tabs.push_back(contents_container_);

  gfx::Rect vertical_tab_strip_bounds = vertical_layout_rect_;
  vertical_tab_strip_bounds.set_y(views_next_to_vertical_tabs.front()->y());
  if (contents_separator_ &&
      views_next_to_vertical_tabs.front() == bookmark_bar_) {
    vertical_tab_strip_host_->SetBorder(
        views::CreateEmptyBorder(gfx::Insets().set_top(
            contents_separator_->GetPreferredSize().height())));
  } else {
    vertical_tab_strip_host_->SetBorder(nullptr);
  }

  vertical_tab_strip_bounds.set_width(
      vertical_tab_strip_host_->GetPreferredSize().width());
  vertical_tab_strip_bounds.set_height(
      views_next_to_vertical_tabs.back()->bounds().bottom() -
      vertical_tab_strip_bounds.y());
  vertical_tab_strip_host_->SetBoundsRect(vertical_tab_strip_bounds);
}

void BraveBrowserViewLayout::LayoutSidePanelView(
    views::View* side_panel,
    gfx::Rect& contents_container_bounds) {
  // We don't want to do any special layout for brave's sidebar (which
  // is the parent of chromium's side panel). We simply
  // use flex layout to put it to the side of the content view.
  return;
}

int BraveBrowserViewLayout::LayoutTabStripRegion(int top) {
  if (!base::FeatureList::IsEnabled(tabs::features::kBraveVerticalTabs))
    return BrowserViewLayout::LayoutTabStripRegion(top);

  if (tabs::features::ShouldShowVerticalTabs(browser_view_->browser())) {
    // In case we're using vertical tabstrip, we can decide the position
    // after we finish laying out views in top container.
    return top;
  }

  return BrowserViewLayout::LayoutTabStripRegion(top);
}

int BraveBrowserViewLayout::LayoutBookmarkAndInfoBars(int top,
                                                      int browser_view_y) {
  if (!vertical_tab_strip_host_ || !ShouldPushBookmarkBarForVerticalTabs())
    return BrowserViewLayout::LayoutBookmarkAndInfoBars(top, browser_view_y);

  auto new_rect = vertical_layout_rect_;
  new_rect.Inset(gfx::Insets().set_left(
      vertical_tab_strip_host_->GetPreferredSize().width()));
  base::AutoReset resetter(&vertical_layout_rect_, new_rect);
  return BrowserViewLayout::LayoutBookmarkAndInfoBars(top, browser_view_y);
}

int BraveBrowserViewLayout::LayoutInfoBar(int top) {
  if (!vertical_tab_strip_host_)
    return BrowserViewLayout::LayoutInfoBar(top);

  auto new_rect = vertical_layout_rect_;
  new_rect.Inset(gfx::Insets().set_left(
      vertical_tab_strip_host_->GetPreferredSize().width()));
  base::AutoReset resetter(&vertical_layout_rect_, new_rect);
  return BrowserViewLayout::LayoutInfoBar(top);
}

void BraveBrowserViewLayout::LayoutContentsContainerView(int top, int bottom) {
  if (!vertical_tab_strip_host_) {
    BrowserViewLayout::LayoutContentsContainerView(top, bottom);
    return;
  }

  auto new_rect = vertical_layout_rect_;
  new_rect.Inset(gfx::Insets().set_left(
      vertical_tab_strip_host_->GetPreferredSize().width()));
  base::AutoReset resetter(&vertical_layout_rect_, new_rect);
  return BrowserViewLayout::LayoutContentsContainerView(top, bottom);
}

bool BraveBrowserViewLayout::ShouldPushBookmarkBarForVerticalTabs() {
  DCHECK(vertical_tab_strip_host_)
      << "This method is used only when vertical tab strip host is set";

  // This can happen when bookmarks bar is visible on NTP. In this case
  // we should lay out vertical tab strip next to bookmarks bar so that
  // the tab strip doesn't move when changing the active tab.
  return bookmark_bar_ &&
         !browser_view_->browser()->profile()->GetPrefs()->GetBoolean(
             bookmarks::prefs::kShowBookmarkBar) &&
         delegate_->IsBookmarkBarVisible();
}
