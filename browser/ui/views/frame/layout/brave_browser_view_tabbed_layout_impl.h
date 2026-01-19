// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_UI_VIEWS_FRAME_LAYOUT_BRAVE_BROWSER_VIEW_TABBED_LAYOUT_IMPL_H_
#define BRAVE_BROWSER_UI_VIEWS_FRAME_LAYOUT_BRAVE_BROWSER_VIEW_TABBED_LAYOUT_IMPL_H_

#include <memory>

#include "chrome/browser/ui/views/frame/layout/browser_view_tabbed_layout_impl.h"

class SidebarContainerView;

// Provides a specialized layout implementation for Brave tabbed browsers
// using the new layout architecture (BrowserViewTabbedLayoutImpl).
// This class extends BrowserViewTabbedLayoutImpl with Brave-specific features
// such as vertical tabs, sidebar, and custom content margins.
class BraveBrowserViewTabbedLayoutImpl : public BrowserViewTabbedLayoutImpl {
 public:
  BraveBrowserViewTabbedLayoutImpl(
      std::unique_ptr<BrowserViewLayoutDelegate> delegate,
      Browser* browser,
      BrowserViewLayoutViews views);
  ~BraveBrowserViewTabbedLayoutImpl() override;

  views::View* contents_container() { return views().contents_container; }

  // Returns the ideal sidebar width, given the current available width. Used
  // for determining the target width in sidebar width animations.
  int GetIdealSideBarWidth() const;
  int GetIdealSideBarWidth(int available_width) const;

  // BrowserViewTabbedLayoutImpl overrides:
  gfx::Size GetMinimumSize(const views::View* host) const override;
  ProposedLayout CalculateProposedLayout(
      const BrowserLayoutParams& params) const override;
  gfx::Rect CalculateTopContainerLayout(ProposedLayout& layout,
                                        BrowserLayoutParams params,
                                        bool needs_exclusion) const override;
  void DoPostLayoutVisualAdjustments(
      const BrowserLayoutParams& params) override;
  TopSeparatorType GetTopSeparatorType() const override;

 private:
  void CalculateBraveVerticalTabStripLayout(
      ProposedLayout& layout,
      const BrowserLayoutParams& params) const;
  void CalculateSideBarLayout(ProposedLayout& layout,
                              const BrowserLayoutParams& params) const;
  void InsetContentsContainerBounds(ProposedLayout& layout) const;

  void UpdateInsetsForVerticalTabStrip();
  void UpdateMarginsForSideBar();

  gfx::Insets GetContentsMargins() const;
  bool ShouldPushBookmarkBarForVerticalTabs() const;
  gfx::Insets GetInsetsConsideringVerticalTabHost() const;

#if BUILDFLAG(IS_MAC)
  gfx::Insets AddFrameBorderInsets(const gfx::Insets& insets) const;
  gfx::Insets AddVerticalTabFrameBorderInsets(const gfx::Insets& insets) const;
#endif
};

#endif  // BRAVE_BROWSER_UI_VIEWS_FRAME_LAYOUT_BRAVE_BROWSER_VIEW_TABBED_LAYOUT_IMPL_H_
