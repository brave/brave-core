// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_UI_VIEWS_FRAME_BRAVE_BROWSER_VIEW_LAYOUT_H_
#define BRAVE_BROWSER_UI_VIEWS_FRAME_BRAVE_BROWSER_VIEW_LAYOUT_H_

#include "chrome/browser/ui/views/frame/browser_view_layout.h"

class SidebarContainerView;

class BraveBrowserViewLayout : public BrowserViewLayout {
 public:
  using BrowserViewLayout::BrowserViewLayout;
  ~BraveBrowserViewLayout() override;

  void set_contents_background(views::View* contents_background) {
    contents_background_ = contents_background;
  }

  void set_vertical_tab_strip_host(views::View* vertical_tab_strip_host) {
    vertical_tab_strip_host_ = vertical_tab_strip_host;
  }

  void set_reader_mode_toolbar(views::View* reader_mode_toolbar) {
    reader_mode_toolbar_ = reader_mode_toolbar;
  }

  void set_sidebar_container(SidebarContainerView* sidebar_container) {
    sidebar_container_ = sidebar_container;
  }

  void set_sidebar_separator(views::View* sidebar_separator) {
    sidebar_separator_ = sidebar_separator;
  }

  // Returns the ideal sidebar width, given the current available width. Used
  // for determining the target width in sidebar width animations.
  int GetIdealSideBarWidth() const;
  int GetIdealSideBarWidth(int available_width) const;

  // BrowserViewLayout:
  void Layout(views::View* host) override;
  void LayoutSidePanelView(views::View* side_panel,
                           gfx::Rect& contents_container_bounds) override;
  int LayoutTabStripRegion(int top) override;
  int LayoutBookmarkAndInfoBars(int top, int browser_view_y) override;
  int LayoutInfoBar(int top) override;
  void LayoutContentsContainerView(int top, int bottom) override;

 private:
  void LayoutVerticalTabs();
  void LayoutSideBar(gfx::Rect& contents_bounds);
  void LayoutReaderModeToolbar(gfx::Rect& contents_bounds);
  gfx::Insets GetContentsMargins() const;
  bool IsReaderModeToolbarVisible() const;
  bool IsFullscreenForTab() const;
  bool IsFullscreenForBrowser() const;
  bool ShouldPushBookmarkBarForVerticalTabs();
  gfx::Insets GetInsetsConsideringVerticalTabHost();
  void UpdateContentsContainerInsets(gfx::Rect& contents_container_bounds);

#if BUILDFLAG(IS_MAC)
  gfx::Insets AdjustInsetsConsideringFrameBorder(const gfx::Insets& insets);
#endif

  raw_ptr<views::View, DanglingUntriaged> vertical_tab_strip_host_ = nullptr;
  raw_ptr<views::View, DanglingUntriaged> reader_mode_toolbar_ = nullptr;
  raw_ptr<SidebarContainerView, DanglingUntriaged> sidebar_container_ = nullptr;
  raw_ptr<views::View> sidebar_separator_ = nullptr;
  raw_ptr<views::View> contents_background_ = nullptr;
};

#endif  // BRAVE_BROWSER_UI_VIEWS_FRAME_BRAVE_BROWSER_VIEW_LAYOUT_H_
