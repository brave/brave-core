/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_VIEWS_FRAME_LAYOUT_BRAVE_BROWSER_VIEW_TABBED_LAYOUT_IMPL_H_
#define BRAVE_BROWSER_UI_VIEWS_FRAME_LAYOUT_BRAVE_BROWSER_VIEW_TABBED_LAYOUT_IMPL_H_

#include <memory>
#include <optional>

#include "chrome/browser/ui/views/frame/layout/browser_view_tabbed_layout_impl.h"
#include "ui/gfx/geometry/insets.h"
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
  // between the contents area and the sidebar control view.  The upstream panel
  // animates its open/close slide against the browser edge; this translates the
  // whole slide by a constant offset so it instead plays out against the inner
  // edge of |sidebar_bounds|, ending flush with the sidebar at full open.
  // |visual_client_area| is the upstream anchor rect (the same one used to
  // compute |panel_bounds|), needed to derive that offset.
  static gfx::Rect ComputeAdjustedPanelBounds(
      bool sidebar_leading,
      const gfx::Rect& sidebar_bounds,
      const gfx::Rect& panel_bounds,
      const gfx::Rect& visual_client_area);
  // Returns the adjusted infobar bounds, resetting the width to
  // |full_window_width| (undoing any panel-driven narrowing upstream applies)
  // and then insetting by |vtab_insets| when vertical tabs are visible.
  static gfx::Rect ComputeAdjustedInfobarBounds(
      const gfx::Rect& current_bounds,
      int full_window_width,
      std::optional<gfx::Insets> vtab_insets);

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
  void ConfigureTopContainerBackground(
      const BrowserLayoutParams& params,
      CustomCornersBackground* background) override;
  void DoPreLayoutComputations(const BrowserLayoutParams& params) override;
  void DoPostLayoutVisualAdjustments(
      const BrowserLayoutParams& params) override;
  // SeparatorInfo is the upstream's private nested type. Friend access (added
  // via the chromium_src .h shadow) lets this subclass name it here; the body
  // of the override lives in
  // chromium_src/.../browser_view_tabbed_layout_impl.cc where the struct
  // definition is visible from the included upstream .cc.
  SeparatorInfo CalculateSeparatorInfo() const override;
  int GetHorizontalTabStripLeadingMargin(
      const BrowserLayoutParams& params) const override;

  // Test-only accessors that expose individual SeparatorInfo bools without
  // requiring the caller to see the upstream's private struct definition.
  // Implemented alongside CalculateSeparatorInfo() in the chromium_src shadow.
  bool GetTopContainerSeparatorForTesting() const;
  bool GetMultiContentsSeparatorForTesting() const;
  bool GetShadowBoxForTesting() const;

 private:
  void CalculateBraveVerticalTabStripLayout(
      ProposedLayout& layout,
      const BrowserLayoutParams& params) const;
  void CalculateSideBarLayout(ProposedLayout& layout,
                              const BrowserLayoutParams& params) const;
  void InsetContentsContainerBounds(ProposedLayout& layout) const;
  void AdjustInfobarLayout(ProposedLayout& layout,
                           const BrowserLayoutParams params) const;

  void UpdateInsetsForVerticalTabStrip();

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
};

#endif  // BRAVE_BROWSER_UI_VIEWS_FRAME_LAYOUT_BRAVE_BROWSER_VIEW_TABBED_LAYOUT_IMPL_H_
