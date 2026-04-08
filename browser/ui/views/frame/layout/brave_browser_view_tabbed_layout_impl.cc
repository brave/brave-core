/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/frame/layout/brave_browser_view_tabbed_layout_impl.h"

#include <algorithm>
#include <limits>
#include <utility>

#include "base/check.h"
#include "base/check_is_test.h"
#include "brave/browser/ui/views/frame/brave_browser_view.h"
#include "brave/browser/ui/views/frame/brave_contents_view_util.h"
#include "brave/browser/ui/views/sidebar/sidebar_container_view.h"
#include "brave/browser/ui/views/tabs/vertical_tab_utils.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_window/public/browser_window_features.h"
#include "chrome/browser/ui/exclusive_access/exclusive_access_manager.h"
#include "chrome/browser/ui/exclusive_access/fullscreen_controller.h"
#include "chrome/browser/ui/tabs/features.h"
#include "chrome/browser/ui/views/bookmarks/bookmark_bar_view.h"
#include "chrome/browser/ui/views/frame/browser_view.h"
#include "chrome/browser/ui/views/frame/layout/browser_view_layout_delegate.h"
#include "chrome/browser/ui/views/infobars/infobar_container_view.h"
#include "components/bookmarks/common/bookmark_pref_names.h"
#include "ui/views/border.h"
#include "ui/views/view_class_properties.h"

BraveBrowserViewTabbedLayoutImpl::BraveBrowserViewTabbedLayoutImpl(
    std::unique_ptr<BrowserViewLayoutDelegate> delegate,
    Browser* browser,
    BrowserViewLayoutViews views)
    : BrowserViewTabbedLayoutImpl(std::move(delegate),
                                  browser,
                                  std::move(views)) {}

BraveBrowserViewTabbedLayoutImpl::~BraveBrowserViewTabbedLayoutImpl() = default;

int BraveBrowserViewTabbedLayoutImpl::GetIdealSideBarWidth() const {
  if (!views().sidebar_container) {
    return 0;
  }

  return GetIdealSideBarWidth(views().contents_container->width() +
                              GetContentsMargins().width() +
                              views().sidebar_container->width());
}

int BraveBrowserViewTabbedLayoutImpl::GetIdealSideBarWidth(
    int available_width) const {
  if (!views().sidebar_container) {
    return 0;
  }

  int sidebar_width = views().sidebar_container->GetPreferredSize().width();

  // The sidebar can take up the entire space for fullscreen.
  if (sidebar_width == std::numeric_limits<int>::max()) {
    return available_width;
  }

  // The sidebar should take up no more than 80% of the content area.
  return std::min<int>(available_width * 0.8, sidebar_width);
}

gfx::Size BraveBrowserViewTabbedLayoutImpl::GetMinimumSize(
    const views::View* host) const {
  // Start with the parent's minimum size calculation
  gfx::Size min_size = BrowserViewTabbedLayoutImpl::GetMinimumSize(host);

  // Add sidebar width if present
  if (views().sidebar_container &&
      views().sidebar_container->IsSidebarVisible()) {
    const int sidebar_min_width =
        views().sidebar_container->GetMinimumSize().width();
    min_size.set_width(min_size.width() + sidebar_min_width);
  }

  return min_size;
}

BrowserViewTabbedLayoutImpl::ProposedLayout
BraveBrowserViewTabbedLayoutImpl::CalculateProposedLayout(
    const BrowserLayoutParams& params) const {
  // Reserve vertical tab space at the window edge first, then sidebar space
  // adjacent to it, so the upstream layout sees the doubly-reduced area.
  //
  // Placement:
  //   CalculateBraveVerticalTabStripLayout() uses original `params` → VT at
  //     the window edge (e.g. right side: [W-VT, W]).
  //   CalculateSidebarInLayout() uses `vt_adjusted` → sidebar just inside VT
  //     (e.g. right side: [W-VT-S, W-VT]).
  //
  // Result for right-side both: [content][sidebar][VT]
  // Result for left-side  both: [VT][sidebar][content]
  const BrowserLayoutParams vt_adjusted = AdjustParamsForVerticalTabs(params);
  const BrowserLayoutParams adjusted_params =
      AdjustParamsForSidebar(vt_adjusted);

  // Get the base layout from parent class using the fully adjusted params.
  ProposedLayout layout =
      BrowserViewTabbedLayoutImpl::CalculateProposedLayout(adjusted_params);

  // The background must span sidebar + VT as well, so pass the original
  // (un-narrowed) params, not adjusted_params.
  CalculateContentsBackgroundLayout(layout, params);

  // The upstream layout used adjusted_params, so it placed the infobar only
  // over the narrowed content area. Pass vt_adjusted so the infobar is
  // expanded to cover the sidebar too (but not the VT strip).
  CalculateInfobarLayout(layout, vt_adjusted);

  // Pass `vt_adjusted` so that the sidebar's outer edge aligns with the inner
  // edge of the VT strip reservation (sidebar is adjacent to, not at the
  // window edge).
  CalculateSidebarInLayout(layout, vt_adjusted);

  // Proposed layout for the Brave vertical tab strip host.
  // Pass original `params` so the VT strip is placed at the window edge.
  CalculateBraveVerticalTabStripLayout(layout, params);

  // Update contents container bounds considering other views like sidebar and
  // vertical tab strip. when the other views are visible, contents container's
  // final bounds will be smaller than the original bounds.
  InsetContentsContainerBounds(layout);

  // Retrieve contents container proposed bounds.
  auto* contents_layout = layout.GetLayoutFor(views().contents_container);
  CHECK(contents_layout);

  if (views().webui_tab_strip && views().webui_tab_strip->GetVisible()) {
    // The WebUI tab strip container should "push" the tab contents down without
    // resizing it.
    contents_layout->bounds.Inset(
        gfx::Insets().set_bottom(-views().webui_tab_strip->size().height()));
  }

  // Mirroring all views that affected by vertical tab alignment in RTL mode
  // as vertical tab/sidebar follow user's setting for their alignment.
  // Each views' mirrored bounds are what we're seeing in RTL mode.
  contents_layout->bounds =
      views().browser_view->GetMirroredRect(contents_layout->bounds);
  if (auto* bookmark_layout = layout.GetLayoutFor(views().bookmark_bar)) {
    bookmark_layout->bounds =
        views().browser_view->GetMirroredRect(bookmark_layout->bounds);
  }
  if (auto* infobar_layout = layout.GetLayoutFor(views().infobar_container)) {
    infobar_layout->bounds =
        views().browser_view->GetMirroredRect(infobar_layout->bounds);
  }
  if (auto* sidebar_layout = layout.GetLayoutFor(views().sidebar_container)) {
    sidebar_layout->bounds =
        views().browser_view->GetMirroredRect(sidebar_layout->bounds);
  }
  if (auto* vertical_tab_host_layout =
          layout.GetLayoutFor(views().vertical_tab_strip_host)) {
    vertical_tab_host_layout->bounds =
        views().browser_view->GetMirroredRect(vertical_tab_host_layout->bounds);
  }

  return layout;
}

gfx::Rect BraveBrowserViewTabbedLayoutImpl::CalculateTopContainerLayout(
    ProposedLayout& layout,
    BrowserLayoutParams params,
    bool needs_exclusion) const {
  // AdjustParamsForSidebar() and AdjustParamsForVerticalTabs() narrowed
  // visual_client_area to reserve space for Brave-specific UIs. The top
  // container (toolbar, tab strip, bookmark bar) must span the full window
  // width, so restore the full width here.
  //
  // The upstream caller passes params via InLocalCoordinates(), which sets
  // visual_client_area.x() = 0 regardless of which side was reserved.  Both
  // left-side and right-side reservations therefore appear as width reductions
  // with x=0.  We always expand from the right (leading=false) to restore
  // the width without moving x.
  //
  // After computing the layout, GetTopContainerBoundsInParent() adds
  // parent_params.va.OffsetFromOrigin() back.  For left-side panels this
  // offset is non-zero (= total left reservation), which would shift the top
  // container to the right.  We pre-subtract that offset from the returned
  // bounds so the final parent-coord position is x=0.
  //
  // params is taken by value so all modifications are local to this call.
  int left_reservation = 0;

  if (views().sidebar_container) {
    // Use the raw preferred width; GetIdealSideBarWidth() would receive the
    // already-narrowed width, giving the wrong cap. The max_int sentinel means
    // the sidebar is full-screen (sidebar_width == available width), so no
    // un-inset is needed.
    int sidebar_width = views().sidebar_container->GetPreferredSize().width();
    if (sidebar_width > 0 && sidebar_width != std::numeric_limits<int>::max()) {
      int reserve = sidebar_width;
      params.InsetHorizontal(-reserve, /*leading=*/false);
      if (views().sidebar_container->sidebar_on_left()) {
        left_reservation += reserve;
      }
    }
  }

  if (delegate().ShouldShowVerticalTabs() && views().vertical_tab_strip_host) {
    gfx::Insets mac_insets;
#if BUILDFLAG(IS_MAC)
    mac_insets = AddVerticalTabFrameBorderInsets(mac_insets);
#endif
    const int vt_width =
        views().vertical_tab_strip_host->GetPreferredSize().width() +
        mac_insets.width();
    if (vt_width > 0) {
      params.InsetHorizontal(-vt_width, /*leading=*/false);
      if (!delegate().IsVerticalTabOnRight()) {
        left_reservation += vt_width;
      }
    }
  }

  // Get base layout from parent with the fully-restored width.
  gfx::Rect bounds = BrowserViewTabbedLayoutImpl::CalculateTopContainerLayout(
      layout, params, needs_exclusion);

  // Compensate for the parent-coord translation that
  // GetTopContainerBoundsInParent will apply.
  if (left_reservation > 0) {
    bounds.Offset(-left_reservation, 0);
  }

  if (!delegate().ShouldShowVerticalTabs()) {
    return bounds;
  }

  // Adjust bookmark bar layout if vertical tabs are shown. i.e. sets insets to
  // bookmark_bar considering vertical tab strip. On macOS, the insets can have
  // bottom insets but it doesn't need for info bar.
  if (ShouldPushBookmarkBarForVerticalTabs()) {
    auto* bookmark_layout = layout.GetLayoutFor(views().bookmark_bar);
    // bookmark layout should be populated by upstream's logic.
    CHECK(bookmark_layout);
    if (bookmark_layout && bookmark_layout->visibility.value_or(true)) {
      gfx::Insets insets = GetInsetsConsideringVerticalTabHost();
      insets.set_bottom(0);
      bookmark_layout->bounds.Inset(insets);
    }
  }

  return bounds;
}

void BraveBrowserViewTabbedLayoutImpl::DoPostLayoutVisualAdjustments(
    const BrowserLayoutParams& params) {
  BrowserViewTabbedLayoutImpl::DoPostLayoutVisualAdjustments(params);
  UpdateInsetsForVerticalTabStrip();
  UpdateMarginsForSideBar();

  if (delegate().ShouldDrawVerticalTabStrip()) {
    return;
  }

  auto* const toolbar_background =
      static_cast<CustomCornersBackground*>(views().toolbar->background());
  CustomCornersBackground::Corners toolbar_corners;
  toolbar_corners.upper_trailing.type =
      CustomCornersBackground::CornerType::kRoundedWithBackground;
  toolbar_corners.upper_leading.type =
      CustomCornersBackground::CornerType::kRoundedWithBackground;
  toolbar_background->SetCorners(toolbar_corners);
}

BrowserViewTabbedLayoutImpl::TopSeparatorType
BraveBrowserViewTabbedLayoutImpl::GetTopSeparatorType() const {
  // Return kNone when there is no visible top UI (toolbar and bookmark bar).
  // This fixes a 1px visible separator at the top of the contents view when in
  // browser fullscreen, where the top chrome is hidden.
  if (!delegate().IsToolbarVisible() && !delegate().IsBookmarkBarVisible()) {
    return TopSeparatorType::kNone;
  }

  // Get the upstream separator type as a starting point. The top separator is
  // a visual line that divides the browser's top UI (toolbar, tabs) from the
  // main content area below it.
  auto top_separator_type = BrowserViewTabbedLayoutImpl::GetTopSeparatorType();

  // Handle the special case where Brave uses rounded corners for the web view.
  // The upstream implementation may return kMultiContents which positions the
  // separator at the contents container boundary, or kNone indicating no
  // separator should be drawn.
  if (top_separator_type == TopSeparatorType::kNone ||
      top_separator_type == TopSeparatorType::kMultiContents) {
    // When rounded corners are enabled, we add padding/margins around the
    // MultiContentsView to create space for the rounded corners and shadow.
    // The kMultiContents separator would only span the width of the contents
    // container (excluding the padding), creating an awkward visual gap.
    //
    // With rounded corners: Return kNone - the separator is not needed since
    //   the rounded corners and shadow provide sufficient visual separation.
    //
    // Without rounded corners: Return kTopContainer - draw the separator at
    //   the top container boundary instead, ensuring it spans the full browser
    //   window width for a clean visual divider.
    return delegate().ShouldUseBraveWebViewRoundedCornersForContents()
               ? TopSeparatorType::kNone
               : TopSeparatorType::kTopContainer;
  }

  // For all other separator types (e.g., kTopContainer, kBookmarkBar), use the
  // upstream behavior as-is since they already work correctly with Brave's UI.
  return top_separator_type;
}

void BraveBrowserViewTabbedLayoutImpl::CalculateBraveVerticalTabStripLayout(
    ProposedLayout& layout,
    const BrowserLayoutParams& params) const {
  if (!IsParentedTo(views().vertical_tab_strip_host, views().browser_view)) {
    return;
  }

  // This is Brave specific view so the layout shouldn't be populated by
  CHECK(!layout.GetLayoutFor(views().vertical_tab_strip_host));

  if (!delegate().ShouldShowVerticalTabs()) {
    layout.AddChild(views().vertical_tab_strip_host, gfx::Rect());
    return;
  }

  // Compute the top edge based on the proposed bounds of bookmark/infobar/top
  // container, not the current view bounds.
  auto get_vertical_tabs_top = [&]() -> int {
    if (ShouldPushBookmarkBarForVerticalTabs()) {
      CHECK(views().bookmark_bar);
      auto* bookmark_layout = layout.GetLayoutFor(views().bookmark_bar);
      CHECK(bookmark_layout);
      return bookmark_layout->bounds.y();
    }

    if (delegate().IsInfobarVisible()) {
      CHECK(views().infobar_container);
      const auto* infobar_layout =
          layout.GetLayoutFor(views().infobar_container);
      CHECK(infobar_layout);
      return infobar_layout->bounds.y();
    }

    CHECK(views().top_container);
    auto* top_container_layout = layout.GetLayoutFor(views().top_container);
    CHECK(top_container_layout);
    return top_container_layout->bounds.bottom() - GetContentsMargins().top();
  };

  gfx::Rect vertical_tab_strip_bounds = views().browser_view->GetLocalBounds();
  vertical_tab_strip_bounds.SetVerticalBounds(
      get_vertical_tabs_top(), vertical_tab_strip_bounds.bottom());

  // Account for any additional frame-border insets on Mac.
  gfx::Insets insets;
#if BUILDFLAG(IS_MAC)
  insets = AddVerticalTabFrameBorderInsets(insets);
#endif

  const int width =
      views().vertical_tab_strip_host->GetPreferredSize().width() +
      insets.width();
  // AdjustParamsForVerticalTabs() pre-reserved a strip at the leading/trailing
  // edge of params.visual_client_area (which here is sidebar_adjusted).
  // Place the VT strip exactly in that reserved strip.
  if (delegate().IsVerticalTabOnRight()) {
    vertical_tab_strip_bounds.set_x(params.visual_client_area.right() - width);
  } else {
    vertical_tab_strip_bounds.set_x(params.visual_client_area.x());
  }
  vertical_tab_strip_bounds.set_width(width);

  layout.AddChild(views().vertical_tab_strip_host, vertical_tab_strip_bounds);
}

// static
gfx::Rect BraveBrowserViewTabbedLayoutImpl::RestoreInfobarBoundsForSidebar(
    const gfx::Rect& infobar_bounds,
    const gfx::Rect& vt_adjusted_client_area) {
  return gfx::Rect(vt_adjusted_client_area.x(), infobar_bounds.y(),
                   vt_adjusted_client_area.width(), infobar_bounds.height());
}

// static
gfx::Rect BraveBrowserViewTabbedLayoutImpl::ComputeContentsBackgroundBounds(
    const gfx::Rect& original_visual_client_area,
    const gfx::Rect& contents_container_bounds) {
  return gfx::Rect(
      original_visual_client_area.x(), contents_container_bounds.y(),
      original_visual_client_area.width(), contents_container_bounds.height());
}

BrowserLayoutParams BraveBrowserViewTabbedLayoutImpl::AdjustParamsForSidebar(
    const BrowserLayoutParams& params) const {
  if (!views().sidebar_container) {
    return params;
  }
  return AdjustParamsForSidebar(
      params, views().sidebar_container->sidebar_on_left(),
      GetIdealSideBarWidth(params.visual_client_area.width()));
}

// static
BrowserLayoutParams BraveBrowserViewTabbedLayoutImpl::AdjustParamsForSidebar(
    const BrowserLayoutParams& params,
    bool sidebar_on_left,
    int sidebar_width) {
  if (sidebar_width <= 0) {
    return params;
  }

  gfx::Insets insets;
  if (sidebar_on_left) {
    insets.set_left(sidebar_width);
  } else {
    insets.set_right(sidebar_width);
  }
  return params.WithInsets(insets);
}

BrowserLayoutParams
BraveBrowserViewTabbedLayoutImpl::AdjustParamsForVerticalTabs(
    const BrowserLayoutParams& params) const {
  if (!delegate().ShouldShowVerticalTabs() ||
      !views().vertical_tab_strip_host) {
    return params;
  }

  gfx::Insets mac_insets;
#if BUILDFLAG(IS_MAC)
  mac_insets = AddVerticalTabFrameBorderInsets(mac_insets);
#endif
  const int width =
      views().vertical_tab_strip_host->GetPreferredSize().width() +
      mac_insets.width();

  return AdjustParamsForVerticalTabs(params, delegate().IsVerticalTabOnRight(),
                                     width);
}

// static
BrowserLayoutParams
BraveBrowserViewTabbedLayoutImpl::AdjustParamsForVerticalTabs(
    const BrowserLayoutParams& params,
    bool vt_on_right,
    int vt_width) {
  if (vt_width <= 0) {
    return params;
  }

  gfx::Insets insets;
  if (vt_on_right) {
    insets.set_right(vt_width);
  } else {
    insets.set_left(vt_width);
  }

  return params.WithInsets(insets);
}

void BraveBrowserViewTabbedLayoutImpl::CalculateInfobarLayout(
    ProposedLayout& layout,
    const BrowserLayoutParams& vt_adjusted) const {
  if (!IsParentedTo(views().infobar_container, views().browser_view) ||
      !delegate().IsInfobarVisible()) {
    return;
  }

  auto* infobar_layout = layout.GetLayoutFor(views().infobar_container);
  CHECK(infobar_layout);
  infobar_layout->bounds = RestoreInfobarBoundsForSidebar(
      infobar_layout->bounds, vt_adjusted.visual_client_area);
}

void BraveBrowserViewTabbedLayoutImpl::CalculateContentsBackgroundLayout(
    ProposedLayout& layout,
    const BrowserLayoutParams& params) const {
  if (!IsParentedTo(views().contents_background, views().browser_view)) {
    return;
  }

  auto* contents_layout = layout.GetLayoutFor(views().contents_container);
  CHECK(contents_layout);

  // This is Brave specific view so the layout shouldn't be populated by
  // upstream's logic.
  CHECK(!layout.GetLayoutFor(views().contents_background));
  layout.AddChild(views().contents_background,
                  ComputeContentsBackgroundBounds(params.visual_client_area,
                                                  contents_layout->bounds));
}

void BraveBrowserViewTabbedLayoutImpl::CalculateSidebarInLayout(
    ProposedLayout& layout,
    const BrowserLayoutParams& params) const {
  if (!IsParentedTo(views().sidebar_container, views().browser_view)) {
    return;
  }

  // Use the content area's vertical extent so the sidebar spans the same
  // height as the web content (below toolbar, bookmarks, etc.).
  auto* contents_layout = layout.GetLayoutFor(views().contents_container);
  CHECK(contents_layout);
  const int top = contents_layout->bounds.y();
  const int height = contents_layout->bounds.height();

  gfx::Rect sidebar_bounds;
  const bool on_left = views().sidebar_container->sidebar_on_left();
  const int sidebar_width =
      GetIdealSideBarWidth(params.visual_client_area.width());

  if (sidebar_width > 0) {
    const int sidebar_x =
        on_left ? params.visual_client_area.x()
                : params.visual_client_area.right() - sidebar_width;
    sidebar_bounds = gfx::Rect(sidebar_x, top, sidebar_width, height);
  }

  // This is a Brave-specific view; the upstream layout must not have populated
  // it already.
  CHECK(!layout.GetLayoutFor(views().sidebar_container));
  layout.AddChild(views().sidebar_container, sidebar_bounds);
}

void BraveBrowserViewTabbedLayoutImpl::InsetContentsContainerBounds(
    ProposedLayout& layout) const {
  auto* contents_layout = layout.GetLayoutFor(views().contents_container);
  if (!contents_layout) {
    return;
  }

  gfx::Rect contents_container_bounds = contents_layout->bounds;

  // Control contents's margin with sidebar & vertical tab state.
  gfx::Insets contents_margins = GetContentsMargins();

  // Don't need to have additional contents margin for rounded corners
  // in tab-initiated fullscreen. Web contents occupies whole screen.
  if (delegate().IsFullscreenForTab()) {
    contents_container_bounds.Inset(contents_margins);
    contents_layout->bounds = contents_container_bounds;
    return;
  }

  // In rounded corners mode, we need to include a little margin so we have
  // somewhere to draw the shadow.
  int contents_margin_for_rounded_corners =
      delegate().ShouldUseBraveWebViewRoundedCornersForContents()
          ? delegate().GetRoundedCornersWebViewMargin()
          : 0;

  // Due to vertical tab's padding(tabs::kMarginForVerticalTabContainers), we
  // can see some space between vertical tab and contents. However, If we don't
  // have margin from contents, vertical tab side contents shadow isn't visible.
  // So, having half of margin from vertical tab and half from contents.
  if (delegate().ShouldShowVerticalTabs() &&
      (views().vertical_tab_strip_host &&
       views().vertical_tab_strip_host->GetPreferredSize().width() != 0) &&
      !delegate().IsFullscreenForBrowser()) {
    const int margin_with_vertical_tab =
        delegate().ShouldUseBraveWebViewRoundedCornersForContents()
            ? (tabs::kMarginForVerticalTabContainers / 2)
            : 0;
    if (delegate().IsVerticalTabOnRight()) {
      contents_margins.set_right(margin_with_vertical_tab);
    } else {
      contents_margins.set_left(margin_with_vertical_tab);
    }
  }

  // If side panel is shown, contents container should have margin
  // because panel doesn't have margin.
  if (delegate().IsContentTypeSidePanelVisible()) {
    contents_container_bounds.Inset(contents_margins);
    contents_layout->bounds = contents_container_bounds;
    return;
  }

  // If sidebar UI is not shown, contents container should have margin.
  if (!views().sidebar_container ||
      !views().sidebar_container->IsSidebarVisible()) {
    contents_container_bounds.Inset(contents_margins);
    contents_layout->bounds = contents_container_bounds;
    return;
  }

  // If sidebar UI is only shown, contents container should have margin
  // based on sidebar's position because sidebar UI itself has padding always.
  // If sidebar is shown in left-side, contents container doesn't need its
  // left margin.
  if (views().sidebar_container->sidebar_on_left()) {
    contents_margins.set_left(contents_margin_for_rounded_corners);
  } else {
    contents_margins.set_right(contents_margin_for_rounded_corners);
  }
  contents_container_bounds.Inset(contents_margins);
  contents_layout->bounds = contents_container_bounds;
}

void BraveBrowserViewTabbedLayoutImpl::UpdateInsetsForVerticalTabStrip() {
  // Update visual-only properties after layout has been applied.
  // The vertical tab strip host bounds are handled via ProposedLayout.
  if (!views().vertical_tab_strip_host) {
    return;
  }

  if (!delegate().ShouldShowVerticalTabs()) {
    views().vertical_tab_strip_host->SetBorder(nullptr);
    return;
  }

  gfx::Insets insets;
#if !BUILDFLAG(IS_LINUX)
  // When the bookmark bar is adjacent to the tabstrip, the separator between
  // the bookmark bar and the content area will also be adjacent, instead of
  // above. In order to avoid tabstrip position changes when switching to a
  // different tab, add some spacing as if the separator were above and
  // invisible.
  if (views().top_container_separator &&
      ShouldPushBookmarkBarForVerticalTabs()) {
    insets.set_top(
        views().top_container_separator->GetPreferredSize().height());
  }
#endif  // BUILDFLAG(IS_LINUX)

#if BUILDFLAG(IS_MAC)
  insets = AddVerticalTabFrameBorderInsets(insets);
#endif

  views().vertical_tab_strip_host->SetBorder(
      insets.IsEmpty() ? nullptr : views::CreateEmptyBorder(insets));
}

void BraveBrowserViewTabbedLayoutImpl::UpdateMarginsForSideBar() {
  if (!views().sidebar_container) {
    return;
  }

  gfx::Insets panel_margins = GetContentsMargins();
  const bool on_left = views().sidebar_container->sidebar_on_left();
  if (delegate().ShouldUseBraveWebViewRoundedCornersForContents()) {
    // In rounded mode, there is already a gap between the sidebar and the main
    // contents view, so we only remove from the margin from that side (we need
    // to keep it between the sidebar controls and the sidebar content).
    if (on_left) {
      panel_margins.set_right(0);
    } else {
      panel_margins.set_left(0);
    }
  } else {
    // Side panel doesn't need margin as sidebar UI and contents container
    // will have margins if needed.
    panel_margins.set_left_right(0, 0);
  }

  // V1 wraps the side panel; set margins on it. V2 has no owned panel.
  if (auto* panel = views().sidebar_container->side_panel()) {
    panel->SetProperty(views::kMarginsKey, panel_margins);
  }
}

gfx::Insets BraveBrowserViewTabbedLayoutImpl::GetContentsMargins() const {
  if (!delegate().ShouldUseBraveWebViewRoundedCornersForContents()) {
    return {};
  }

  if (delegate().IsFullscreenForTab()) {
    return {};
  }

  gfx::Insets margins(BraveContentsViewUtil::kMarginThickness);

  // If there is a visible view above the contents container, then there is no
  // need for a top margin.
  if (delegate().ShouldDrawTabStrip() || delegate().IsToolbarVisible() ||
      delegate().IsBookmarkBarVisible() || delegate().IsInfobarVisible()) {
    margins.set_top(0);
  }

  return margins;
}

bool BraveBrowserViewTabbedLayoutImpl::ShouldPushBookmarkBarForVerticalTabs()
    const {
  CHECK(views().vertical_tab_strip_host)
      << "This method is used only when vertical tab strip host is set";

  // This can happen when bookmarks bar is visible on NTP. In this case
  // we should lay out vertical tab strip next to bookmarks bar so that
  // the tab strip doesn't move when changing the active tab.
  return views().bookmark_bar && !delegate().IsBookmarkBarOnByPref() &&
         delegate().IsBookmarkBarVisible();
}

gfx::Insets
BraveBrowserViewTabbedLayoutImpl::GetInsetsConsideringVerticalTabHost() const {
  // This method is used only when vertical tab strip host is set
  CHECK(views().vertical_tab_strip_host);

  gfx::Insets insets;
  if (delegate().IsVerticalTabOnRight()) {
    insets.set_right(
        views().vertical_tab_strip_host->GetPreferredSize().width());
  } else {
    insets.set_left(
        views().vertical_tab_strip_host->GetPreferredSize().width());
  }

#if BUILDFLAG(IS_MAC)
  insets = AddFrameBorderInsets(insets);
#endif

  return insets;
}

#if BUILDFLAG(IS_MAC)
gfx::Insets BraveBrowserViewTabbedLayoutImpl::AddFrameBorderInsets(
    const gfx::Insets& insets) const {
  if (base::FeatureList::IsEnabled(tabs::kBraveVerticalTabStripEmbedded)) {
    return insets;
  }

  // We need more care about frame border when vertical tab is visible.
  // Frame border is not drawn in fullscreen.
  if (!delegate().ShouldShowVerticalTabs() || delegate().IsFullscreen()) {
    return insets;
  }

  // Frame border is drawn on this 1px padding as we set insets to
  // contents container. Otherwise, frame border is drawn on the contents.
  // Why we need this? When vertical tab is floating, vertical tab widget
  // is moved by 1px from the border to prevent overlap with frame border.
  // If the frame border is drawn over the contents, vertical tab widget seems
  // like floating on the contents. See the screenshot at
  // https://github.com/brave/brave-browser/issues/51464. If we give this insets
  // to contents container, frame border is drawn over the background color. So,
  // floated vertical tab widget seems like attached to windows border.
  return insets + gfx::Insets::TLBR(0, 1, 1, 1);
}

gfx::Insets BraveBrowserViewTabbedLayoutImpl::AddVerticalTabFrameBorderInsets(
    const gfx::Insets& insets) const {
  if (base::FeatureList::IsEnabled(tabs::kBraveVerticalTabStripEmbedded)) {
    return insets;
  }

  if (!delegate().ShouldShowVerticalTabs() || delegate().IsFullscreen()) {
    return insets;
  }

  // For frame border drawn by OS. Vertical tabstrip's widget shouldn't cover
  // that line.
  gfx::Insets insets_for_frame_border;
  if (delegate().IsVerticalTabOnRight()) {
    insets_for_frame_border.set_right(1);
  } else {
    insets_for_frame_border.set_left(1);
  }
  insets_for_frame_border.set_bottom(1);

  return insets + insets_for_frame_border;
}
#endif
