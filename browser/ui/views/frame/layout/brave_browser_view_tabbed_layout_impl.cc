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
#include "base/i18n/rtl.h"
#include "brave/browser/ui/views/frame/brave_browser_view.h"
#include "brave/browser/ui/views/sidebar/sidebar_container_view.h"
#include "build/build_config.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_window/public/browser_window_features.h"
#include "chrome/browser/ui/exclusive_access/exclusive_access_manager.h"
#include "chrome/browser/ui/exclusive_access/fullscreen_controller.h"
#include "chrome/browser/ui/layout_constants.h"
#include "chrome/browser/ui/tabs/features.h"
#include "chrome/browser/ui/tabs/tab_style.h"
#include "chrome/browser/ui/views/bookmarks/bookmark_bar_view.h"
#include "chrome/browser/ui/views/frame/browser_view.h"
#include "chrome/browser/ui/views/frame/custom_corners_background.h"
#include "chrome/browser/ui/views/frame/layout/browser_view_layout_delegate.h"
#include "chrome/browser/ui/views/infobars/infobar_container_view.h"
#include "chrome/browser/ui/views/side_panel/side_panel.h"
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

void BraveBrowserViewTabbedLayoutImpl::ConfigureTopContainerBackground(
    const BrowserLayoutParams& params,
    CustomCornersBackground* background) {
  BrowserViewTabbedLayoutImpl::ConfigureTopContainerBackground(params,
                                                               background);
#if BUILDFLAG(IS_LINUX)
  // Curve top container corners like window corner as it's top-most UI
  // in vertical tabs.
  if (delegate().ShouldShowVerticalTabs() &&
      !delegate().ShouldShowWindowTitleForVerticalTabs()) {
    CustomCornersBackground::Corners corners;
    corners.upper_trailing = background->GetWindowCorner(/*upper=*/true);
    corners.upper_leading = background->GetWindowCorner(/*upper=*/true);
    background->SetCorners(corners);
  }
#endif  // BUILDFLAG(IS_LINUX)
}

// static
gfx::Rect BraveBrowserViewTabbedLayoutImpl::ComputeSidebarBounds(
    bool sidebar_leading,
    int sidebar_width,
    int outer_left,
    int outer_right,
    int y,
    int height) {
  const int x = sidebar_leading ? outer_left : outer_right - sidebar_width;
  return gfx::Rect(x, y, sidebar_width, height);
}

// static
gfx::Rect BraveBrowserViewTabbedLayoutImpl::ComputeAdjustedPanelBounds(
    bool sidebar_leading,
    const gfx::Rect& sidebar_bounds,
    const gfx::Rect& panel_bounds,
    const gfx::Rect& visual_client_area) {
  // The upstream layout animates the panel's open/close by sliding panel.x
  // between the browser edge (hidden) and the target width (shown), keeping the
  // bounds width fixed. We translate that whole slide by a constant offset so
  // it plays out against the sidebar's inner edge instead of the browser edge.
  //
  // A constant shift preserves the animation — overwriting panel.x outright
  // would pin the bounds to a fixed rectangle and flatten the slide (the panel
  // would pop to fully-open and overlap the still-animating contents). The
  // offset is the gap between the upstream anchor edge (|visual_client_area|,
  // the same rect upstream used to place |panel_bounds|) and the sidebar's
  // inner edge. Because |sidebar_bounds| is already inset for any vertical tab
  // on the same side, the offset is correct in every VT configuration.
  //
  // The sidebar and the upstream panel share the alignment pref, so
  // `sidebar_leading` matches upstream's `side_panel_leading`.
  gfx::Rect result = panel_bounds;
  if (sidebar_leading) {
    // Slide anchored to the sidebar's trailing (right) edge.
    result.Offset(sidebar_bounds.right() - visual_client_area.x(), 0);
  } else {
    // Slide anchored to the sidebar's leading (left) edge.
    result.Offset(sidebar_bounds.x() - visual_client_area.right(), 0);
  }
  return result;
}

// static
gfx::Rect BraveBrowserViewTabbedLayoutImpl::ComputeAdjustedInfobarBounds(
    const gfx::Rect& current_bounds,
    int full_window_width,
    std::optional<gfx::Insets> vtab_insets) {
  gfx::Rect bounds = current_bounds;
  bounds.set_width(full_window_width);
  if (vtab_insets) {
    bounds.Inset(*vtab_insets);
  }
  return bounds;
}

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
  // Get the base layout from parent class
  ProposedLayout layout =
      BrowserViewTabbedLayoutImpl::CalculateProposedLayout(params);

  // Retrieve contents container proposed bounds.
  auto* contents_layout = layout.GetLayoutFor(views().contents_container);
  CHECK(contents_layout);

  // Handle contents background - contents background should be laid out before
  // other views like sidebar or vertical tab strip in order to cover the entire
  // contents area that contains sidebar. Otherwise, we would have hole between
  // contents background and sidebar when using rounded corners.
  if (views().contents_background && contents_layout) {
    layout.AddChild(views().contents_background, contents_layout->bounds);
  }

  // Apply vertical tab strip insets for contents container BEFORE laying out
  // sidebar, so the sidebar is positioned adjacent to (not underneath) the
  // vertical tab strip when it's on the right. This is because sidebar is laid
  // out depending on the contents_layout->bounds.
  if (views().vertical_tab_strip_host && delegate().ShouldShowVerticalTabs()) {
    // Both vertical tab impls should not be enabled together.
    CHECK(!tabs::IsVerticalTabsFeatureEnabled());
    contents_layout->bounds.Inset(GetInsetsConsideringVerticalTabHost());
  }

  // Handle sidebar and adjust contents container bounds. This should be done
  // BEFORE calling `InsetContentsContainerBounds()` so that the contents
  // container's final bounds is updated considering the sidebar's bounds.
  CalculateSideBarLayout(layout, params);

  // Update contents container bounds considering other views like sidebar and
  // vertical tab strip. when the other views are visible, contents container's
  // final bounds will be smaller than the original bounds.
  InsetContentsContainerBounds(layout);

  // Proposed layout for the Brave vertical tab strip host.
  if (delegate().ShouldShowVerticalTabs()) {
    CalculateBraveVerticalTabStripLayout(layout, params);
  } else if (views().vertical_tab_strip_host) {
    // This is Brave specific view so the layout shouldn't be populated by
    // upstream's logic.
    CHECK(!layout.GetLayoutFor(views().vertical_tab_strip_host));
    layout.AddChild(views().vertical_tab_strip_host, gfx::Rect(), false);
  }

  AdjustInfobarLayout(layout, params);

  return layout;
}

void BraveBrowserViewTabbedLayoutImpl::AdjustInfobarLayout(
    ProposedLayout& layout,
    const BrowserLayoutParams params) const {
  if (!IsParentedTo(views().infobar_container, views().browser_view) ||
      !delegate().IsInfobarVisible()) {
    return;
  }

  auto* infobar_layout = layout.GetLayoutFor(views().infobar_container);
  CHECK(infobar_layout);

  infobar_layout->bounds = ComputeAdjustedInfobarBounds(
      infobar_layout->bounds, params.visual_client_area.width(),
      views().vertical_tab_strip_host
          ? std::make_optional(GetInsetsConsideringVerticalTabHost())
          : std::nullopt);
}

gfx::Rect BraveBrowserViewTabbedLayoutImpl::CalculateTopContainerLayout(
    ProposedLayout& layout,
    BrowserLayoutParams params,
    bool needs_exclusion) const {
  gfx::Rect bounds = BrowserViewTabbedLayoutImpl::CalculateTopContainerLayout(
      layout, params, needs_exclusion);

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

  if (delegate().ShouldDrawVerticalTabStrip()) {
    return;
  }

  auto* const toolbar_background =
      static_cast<CustomCornersBackground*>(views().toolbar->background());
  CustomCornersBackground::Corners toolbar_corners;

#if BUILDFLAG(IS_LINUX)
  if (delegate().ShouldShowVerticalTabs() &&
      !delegate().ShouldShowWindowTitleForVerticalTabs()) {
    // Curve toolbar corners like window corner as toolbar it's top-most UI in
    // vertical tabs.
    toolbar_corners.upper_trailing =
        toolbar_background->GetWindowCorner(/*upper=*/true);
    toolbar_corners.upper_leading =
        toolbar_background->GetWindowCorner(/*upper=*/true);
    toolbar_background->SetCorners(toolbar_corners);
    return;
  }
#endif  // BUILDFLAG(IS_LINUX)

  toolbar_corners.upper_trailing.type =
      CustomCornersBackground::CornerType::kRoundedWithBackground;
  toolbar_corners.upper_leading.type =
      CustomCornersBackground::CornerType::kRoundedWithBackground;
  toolbar_background->SetCorners(toolbar_corners);
}

int BraveBrowserViewTabbedLayoutImpl::GetHorizontalTabStripLeadingMargin(
    const BrowserLayoutParams& params) const {
  // Compact, with no leading exclusion padding (i.e. not an alternate
  // tab-strip layout like split-view): tuck the first tab pill against the
  // caption-button cluster by halving the corner radius. All other cases
  // (non-compact, split-view, etc.) get the upstream margin.
  if (tabs::UseCompactHorizontalTabs() &&
      !views().horizontal_tab_strip_region_view->GetProperty(
          views::kInternalPaddingKey)) {
    return TabStyle::Get()->GetBottomCornerRadius() / 2;
  }
  return BrowserViewTabbedLayoutImpl::GetHorizontalTabStripLeadingMargin(
      params);
}

void BraveBrowserViewTabbedLayoutImpl::CalculateBraveVerticalTabStripLayout(
    ProposedLayout& layout,
    const BrowserLayoutParams& params) const {
  CHECK(views().vertical_tab_strip_host);

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

  const int width = views().vertical_tab_strip_host->GetPreferredSize().width();
  if (!IsVerticalTabStripLeading()) {
    vertical_tab_strip_bounds.set_x(vertical_tab_strip_bounds.right() - width);
  }
  vertical_tab_strip_bounds.set_width(width);

  layout.AddChild(views().vertical_tab_strip_host, vertical_tab_strip_bounds);
}

void BraveBrowserViewTabbedLayoutImpl::CalculateSideBarLayout(
    ProposedLayout& layout,
    const BrowserLayoutParams& params) const {
  if (!views().sidebar_container) {
    return;
  }

  auto* contents_layout = layout.GetLayoutFor(views().contents_container);
  CHECK(contents_layout);

  gfx::Rect contents_bounds = contents_layout->bounds;

  const bool sidebar_leading = IsSidebarLeading();

  // The sidebar is always the outermost element on its side (only the vertical
  // tab, when on the same side, sits further out).
  //
  // All bounds here are in stored coordinates; `leading` is the lowest-X edge
  // (rendered on the visual right in RTL).
  //
  // Desired layout (sidebar trailing):
  //   [vertical_tab] [contents] [panel] [sidebar]
  //   [contents] [panel] [sidebar] [vertical_tab]
  // Desired layout (sidebar leading):
  //   [vertical_tab] [sidebar] [panel] [contents]
  //   [sidebar] [panel] [contents] [vertical_tab]
  //
  // sidebar_container holds only the control view; the upstream side panel is a
  // direct child of browser_view positioned separately by the adjust_panel
  // lambda below.

  // Vertical tab is outermost when on the same side as the sidebar.
  const bool vtab_on_same_side =
      views().vertical_tab_strip_host && delegate().ShouldShowVerticalTabs() &&
      (sidebar_leading == IsVerticalTabStripLeading());
  const int vtab_width =
      vtab_on_same_side
          ? views().vertical_tab_strip_host->GetPreferredSize().width()
          : 0;

  // Outer available edges, inset for any vertical tab on the same side.
  const gfx::Rect browser_bounds = views().browser_view->GetLocalBounds();
  const int outer_left =
      browser_bounds.x() + (sidebar_leading ? vtab_width : 0);
  const int outer_right =
      browser_bounds.right() - (sidebar_leading ? 0 : vtab_width);

  // Sidebar width capped at 80% of the space shared between contents and
  // sidebar (contents_bounds.width()), i.e. the available width minus the vtab
  // and the upstream side panel width. The sidebar control is narrow enough
  // that the cap never fires.
  const int sidebar_width = GetIdealSideBarWidth(contents_bounds.width());

  const gfx::Rect sidebar_bounds = ComputeSidebarBounds(
      sidebar_leading, sidebar_width, outer_left, outer_right,
      contents_bounds.y(), contents_bounds.height());

  // Shift upstream side panels inward so they sit between the contents and the
  // sidebar control.
  auto adjust_panel = [&](SidePanel* panel) {
    if (!panel) {
      return;
    }
    auto* panel_layout = layout.GetLayoutFor(panel);
    if (!panel_layout) {
      return;
    }
    panel_layout->bounds = ComputeAdjustedPanelBounds(
        sidebar_leading, sidebar_bounds, panel_layout->bounds,
        params.visual_client_area);
    // The upstream layout offsets the panel -1px above the contents to overlap
    // the toolbar separator. Brave doesn't need that overlap; align the panel's
    // vertical extent with the contents container instead.
    panel_layout->bounds.set_y(contents_bounds.y());
    panel_layout->bounds.set_height(contents_bounds.height());
  };
  adjust_panel(views().side_panel.get());

  // Reduce contents bounds by the sidebar width on the sidebar side.
  if (sidebar_leading) {
    contents_bounds.Inset(gfx::Insets().set_left(sidebar_width));
  } else {
    contents_bounds.Inset(gfx::Insets().set_right(sidebar_width));
  }

#if BUILDFLAG(IS_MAC)
  // On Mac, an empty-width rect for the contents web view can crash
  // `StatusBubbleViews` (its width is one third of the base view, so set 3
  // to guarantee a minimum width of 1).
  if (contents_bounds.width() <= 0) {
    contents_bounds.set_width(3);
  }
#endif

  contents_layout->bounds = contents_bounds;

  // This is a Brave-specific view; upstream must not have populated it.
  CHECK(views().sidebar_container);
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
    if (IsVerticalTabStripLeading()) {
      contents_margins.set_left(margin_with_vertical_tab);
    } else {
      contents_margins.set_right(margin_with_vertical_tab);
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
  // The contents container doesn't need the full margin on the sidebar side.
  if (IsSidebarLeading()) {
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

  views().vertical_tab_strip_host->SetBorder(
      insets.IsEmpty() ? nullptr : views::CreateEmptyBorder(insets));
}

gfx::Insets BraveBrowserViewTabbedLayoutImpl::GetContentsMargins() const {
  if (!delegate().ShouldUseBraveWebViewRoundedCornersForContents()) {
    return {};
  }

  if (delegate().IsFullscreenForTab()) {
    return {};
  }

  gfx::Insets margins(kRoundedCornersContentsViewMargin);

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

bool BraveBrowserViewTabbedLayoutImpl::IsSidebarLeading() const {
  // In stored coordinates, the leading (lowest-X) edge renders on the visual
  // left in LTR and the visual right in RTL. Flip the pref in RTL so the
  // sidebar always appears on the visual side the user chose.
  return views().sidebar_container->sidebar_on_left() != base::i18n::IsRTL();
}

bool BraveBrowserViewTabbedLayoutImpl::IsVerticalTabStripLeading() const {
  // See IsSideBarLeading() for the polarity rationale.
  return delegate().IsVerticalTabOnRight() == base::i18n::IsRTL();
}

gfx::Insets
BraveBrowserViewTabbedLayoutImpl::GetInsetsConsideringVerticalTabHost() const {
  // This method is used only when vertical tab strip host is set
  CHECK(views().vertical_tab_strip_host);

  gfx::Insets insets;
  if (IsVerticalTabStripLeading()) {
    insets.set_left(
        views().vertical_tab_strip_host->GetPreferredSize().width());
  } else {
    insets.set_right(
        views().vertical_tab_strip_host->GetPreferredSize().width());
  }

  return insets;
}
