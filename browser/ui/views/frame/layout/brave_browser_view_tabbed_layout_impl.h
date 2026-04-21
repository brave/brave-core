/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_VIEWS_FRAME_LAYOUT_BRAVE_BROWSER_VIEW_TABBED_LAYOUT_IMPL_H_
#define BRAVE_BROWSER_UI_VIEWS_FRAME_LAYOUT_BRAVE_BROWSER_VIEW_TABBED_LAYOUT_IMPL_H_

#include <memory>

#include "chrome/browser/ui/views/frame/layout/browser_view_tabbed_layout_impl.h"
#include "ui/gfx/geometry/rect.h"

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

  // Pure geometry helpers used by CalculateSideBarLayout. Exposed as public
  // static so tests can call them directly without special access.
  //
  // Returns the bounds for the sidebar control view given its side, width,
  // the outer (browser-edge) limits already inset for any vertical tab on the
  // same side, and the vertical extents of the contents area.
  static gfx::Rect ComputeSidebarBounds(bool sidebar_on_left,
                                        int sidebar_width,
                                        int outer_left,
                                        int outer_right,
                                        int y,
                                        int height);
  // Returns the adjusted bounds for an upstream side panel so that it sits
  // between the contents area and the sidebar control view.  The panel is
  // shifted inward (toward the centre of the browser) until it is flush with
  // the inner edge of |sidebar_bounds|.
  static gfx::Rect ComputeAdjustedPanelBounds(bool sidebar_on_left,
                                              const gfx::Rect& sidebar_bounds,
                                              const gfx::Rect& panel_bounds);

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

  // Expands the proposed layout to account for the top-overlay reveal
  // animation. When a reveal fraction is present, top views (the tab strip
  // and top container) slide over the contents area; this method offsets
  // those views by the current reveal amount, sizes the focus-mode title
  // bar, repositions the infobar, extends the contents bounds and contents
  // background upward so they begin just below the title bar, and sets the
  // top-container background to the union of the moved top views. When no
  // reveal fraction is present, the focus-mode title bar and top-container
  // background are sized to empty rects. Must be called after
  // `contents_background` has been added to the layout, and before
  // `CalculateSideBarLayout()` so the sidebar picks up the adjusted contents
  // bounds.
  void AdjustLayoutForTopReveal(ProposedLayout& layout) const;

  void UpdateInsetsForVerticalTabStrip();
  void UpdateMarginsForSideBar();

  gfx::Insets GetContentsMargins() const;
  bool ShouldPushBookmarkBarForVerticalTabs() const;
  gfx::Insets GetInsetsConsideringVerticalTabHost() const;

#if BUILDFLAG(IS_MAC)
  gfx::Insets AddFrameBorderInsets(const gfx::Insets& insets) const;
  gfx::Insets AddVerticalTabFrameBorderInsets(const gfx::Insets& insets) const;

  friend class BraveBrowserViewTabbedLayoutImplMacTest;
#endif
};

#endif  // BRAVE_BROWSER_UI_VIEWS_FRAME_LAYOUT_BRAVE_BROWSER_VIEW_TABBED_LAYOUT_IMPL_H_
