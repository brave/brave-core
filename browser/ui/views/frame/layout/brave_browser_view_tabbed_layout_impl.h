/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_VIEWS_FRAME_LAYOUT_BRAVE_BROWSER_VIEW_TABBED_LAYOUT_IMPL_H_
#define BRAVE_BROWSER_UI_VIEWS_FRAME_LAYOUT_BRAVE_BROWSER_VIEW_TABBED_LAYOUT_IMPL_H_

#include <memory>

#include "base/gtest_prod_util.h"
#include "chrome/browser/ui/views/frame/layout/browser_view_tabbed_layout_impl.h"

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

  // Returns a copy of `params` with `visual_client_area` inset to reserve space
  // for the sidebar strip (and optional separator). Call this before passing
  // params to the base-class layout so the upstream side-panel and
  // contents-container are positioned within the already-reduced area.
  BrowserLayoutParams AdjustParamsForSidebar(
      const BrowserLayoutParams& params) const;

  // Returns a copy of `params` with `visual_client_area` inset to reserve space
  // for the vertical tab strip. Call this (chained after
  // AdjustParamsForSidebar) so the upstream layout runs within the area
  // excluding both the sidebar and the vertical tab strip.
  BrowserLayoutParams AdjustParamsForVerticalTabs(
      const BrowserLayoutParams& params) const;

  // Pure-math overload for unit tests (mirrors the AdjustParamsForSidebar
  // pattern). The instance overload above extracts vt_on_right and vt_width
  // from the delegate / view and delegates here.
  static BrowserLayoutParams AdjustParamsForVerticalTabs(
      const BrowserLayoutParams& params,
      bool vt_on_right,
      int vt_width);

  // Pure-math overload: takes the precomputed values so the reservation logic
  // can be exercised in unit tests without constructing real views. The
  // instance overload above extracts these values from views() and delegates
  // here.
  static BrowserLayoutParams AdjustParamsForSidebar(
      const BrowserLayoutParams& params,
      bool sidebar_on_left,
      int sidebar_width);

  // Returns a copy of `infobar_bounds` with x/width replaced by those of
  // `vt_adjusted_client_area`. The infobar must span content + sidebar (the
  // area not occupied by the VT strip), which is exactly what vt_adjusted
  // captures. y and height are preserved from `infobar_bounds`.
  static gfx::Rect RestoreInfobarBoundsForSidebar(
      const gfx::Rect& infobar_bounds,
      const gfx::Rect& vt_adjusted_client_area);

  // Returns the bounds for the contents background view by combining the
  // horizontal span of `original_visual_client_area` (un-narrowed, spanning
  // sidebar + VT) with the vertical extent of `contents_container_bounds`.
  // The background must cover the full area below the top container including
  // sidebar and VT columns, so it always uses the original x/width.
  static gfx::Rect ComputeContentsBackgroundBounds(
      const gfx::Rect& original_visual_client_area,
      const gfx::Rect& contents_container_bounds);

  // Places the contents background view in `layout` spanning the full
  // original `params.visual_client_area` horizontally (covering sidebar + VT)
  // and the vertical extent of the contents_container. Must be called before
  // InsetContentsContainerBounds() so the background is not clipped by
  // rounded-corner margins applied to the web view.
  void CalculateContentsBackgroundLayout(
      ProposedLayout& layout,
      const BrowserLayoutParams& params) const;

  // Expands the infobar bounds to span content + sidebar (the area not occupied
  // by the VT strip). The upstream layout placed the infobar only over the
  // narrowed content area; this restores its horizontal extent to
  // `vt_adjusted.visual_client_area` so it covers the sidebar too.
  void CalculateInfobarLayout(ProposedLayout& layout,
                              const BrowserLayoutParams& vt_adjusted) const;

  // Adds the sidebar container to `layout` at the edge that was reserved by
  // AdjustParamsForSidebar().
  void CalculateSidebarInLayout(ProposedLayout& layout,
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

  friend class BraveBrowserViewTabbedLayoutImplMacTest;
#endif

  FRIEND_TEST_ALL_PREFIXES(InfobarExpansionTest, CoversContentPlusSidebar);
  FRIEND_TEST_ALL_PREFIXES(AdjustParamsForSidebarTest, ReservesCorrectStrip);
  FRIEND_TEST_ALL_PREFIXES(AdjustParamsForVerticalTabsTest,
                           ReservesCorrectStrip);
  FRIEND_TEST_ALL_PREFIXES(CombinedReservationTest, ContentAreaCorrect);
  FRIEND_TEST_ALL_PREFIXES(ContentsBackgroundBoundsTest,
                           SpansFullOriginalWidth);
};

#endif  // BRAVE_BROWSER_UI_VIEWS_FRAME_LAYOUT_BRAVE_BROWSER_VIEW_TABBED_LAYOUT_IMPL_H_
