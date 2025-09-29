// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_UI_VIEWS_FRAME_BRAVE_BROWSER_VIEW_LAYOUT_H_
#define BRAVE_BROWSER_UI_VIEWS_FRAME_BRAVE_BROWSER_VIEW_LAYOUT_H_

#include <memory>

#include "chrome/browser/ui/views/frame/browser_view_layout.h"

class SidebarContainerView;

class BraveBrowserViewLayout : public BrowserViewLayout {
 public:
  BraveBrowserViewLayout(std::unique_ptr<BrowserViewLayoutDelegate> delegate,
                         BrowserView* browser_view,
                         views::View* window_scrim,
                         views::View* top_container,
                         WebAppFrameToolbarView* web_app_frame_toolbar,
                         views::Label* web_app_window_title,
                         TabStripRegionView* tab_strip_region_view,
                         views::View* vertical_tab_strip_container,
                         views::View* toolbar,
                         InfoBarContainerView* infobar_container,
                         views::View* main_container,
                         views::View* contents_container,
                         MultiContentsView* multi_contents_view,
                         views::View* left_aligned_side_panel_separator,
                         views::View* unified_side_panel,
                         views::View* right_aligned_side_panel_separator,
                         views::View* side_panel_rounded_corner,
                         views::View* contents_separator);
  ~BraveBrowserViewLayout() override;

  void set_contents_background(views::View* contents_background) {
    contents_background_ = contents_background;
  }

  void set_vertical_tab_strip_host(views::View* vertical_tab_strip_host) {
    vertical_tab_strip_host_ = vertical_tab_strip_host;
  }

  void set_sidebar_container(SidebarContainerView* sidebar_container) {
    sidebar_container_ = sidebar_container;
  }

  void set_sidebar_separator(views::View* sidebar_separator) {
    sidebar_separator_ = sidebar_separator;
  }

  views::View* contents_container() { return contents_container_; }

  // Returns the ideal sidebar width, given the current available width. Used
  // for determining the target width in sidebar width animations.
  int GetIdealSideBarWidth() const;
  int GetIdealSideBarWidth(int available_width) const;

  // BrowserViewLayout:
  void Layout(views::View* host) override;
  void LayoutTabStripRegion(gfx::Rect& available_bounds) override;
  void LayoutBookmarkBar(gfx::Rect& available_bounds) override;
  void LayoutInfoBar(gfx::Rect& available_bounds) override;
  void LayoutContentsContainerView(const gfx::Rect& available_bounds) override;
  bool IsImmersiveModeEnabledWithoutToolbar() const override;

 private:
  void LayoutVerticalTabs();
  void LayoutSideBar(gfx::Rect& contents_bounds);
  gfx::Insets GetContentsMargins() const;
  bool IsFullscreenForTab() const;
  bool IsFullscreenForBrowser() const;
  bool ShouldPushBookmarkBarForVerticalTabs();
  gfx::Insets GetInsetsConsideringVerticalTabHost();
  void UpdateContentsContainerInsets(gfx::Rect& contents_container_bounds);

#if BUILDFLAG(IS_MAC)
  gfx::Insets AdjustInsetsConsideringFrameBorder(const gfx::Insets& insets);
#endif

  raw_ptr<views::View, DanglingUntriaged> vertical_tab_strip_host_ = nullptr;
  raw_ptr<SidebarContainerView, DanglingUntriaged> sidebar_container_ = nullptr;
  raw_ptr<views::View> sidebar_separator_ = nullptr;
  raw_ptr<views::View> contents_background_ = nullptr;
};

#endif  // BRAVE_BROWSER_UI_VIEWS_FRAME_BRAVE_BROWSER_VIEW_LAYOUT_H_
