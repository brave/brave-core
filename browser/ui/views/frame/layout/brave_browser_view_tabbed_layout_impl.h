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
  // All bounds are in stored (pre-paint-mirroring) coordinates, following the
  // upstream convention: `leading` means the lowest-X edge, which renders on
  // the visual right in RTL.
  //
  // Returns the bounds for the sidebar control view given its leading edge,
  // width, the outer (browser-edge) limits already inset for any vertical tab
  // on the same side, and the vertical extents of the contents area.
  static gfx::Rect ComputeSidebarBounds(bool sidebar_leading,
                                        int sidebar_width,
                                        int outer_left,
                                        int outer_right,
                                        int y,
                                        int height);
  // Returns the adjusted bounds for an upstream side panel so that it sits
  // between the contents area and the sidebar control view.  The panel is
  // shifted inward (toward the centre of the browser) until it is flush with
  // the inner edge of |sidebar_bounds|.
  static gfx::Rect ComputeAdjustedPanelBounds(bool sidebar_leading,
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
  gfx::Rect CalculateTopContainerLayoutImpl(
      ProposedLayout& layout,
      BrowserLayoutParams params,
      bool needs_exclusion,
      bool suppress_top_separator) const override;
  void ConfigureTopContainerBackground(
      const BrowserLayoutParams& params,
      CustomCornersBackground* background) override;
  void DoPostLayoutVisualAdjustments(
      const BrowserLayoutParams& params) override;
  TopSeparatorType GetTopSeparatorType() const override;
  int GetHorizontalTabStripLeadingMargin(
      const BrowserLayoutParams& params) const override;
  bool ShadowOverlayVisible() const override;

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

  // Whether the sidebar / vertical tab strip occupies the leading (lowest-X)
  // edge in stored coordinates. As stored bounds are paint-mirrored in RTL,
  // these flip the user's alignment pref so the visual side always matches
  // the pref. This is the same conversion upstream does for its side panel
  // (`side_panel_leading` in BrowserViewTabbedLayoutImpl).
  bool IsSidebarLeading() const;
  bool IsVerticalTabStripLeading() const;

#if BUILDFLAG(IS_MAC)
  gfx::Insets AddFrameBorderInsets(const gfx::Insets& insets) const;
  gfx::Insets AddVerticalTabFrameBorderInsets(const gfx::Insets& insets) const;

  friend class BraveBrowserViewTabbedLayoutImplMacTest;
#endif
};

#endif  // BRAVE_BROWSER_UI_VIEWS_FRAME_LAYOUT_BRAVE_BROWSER_VIEW_TABBED_LAYOUT_IMPL_H_
