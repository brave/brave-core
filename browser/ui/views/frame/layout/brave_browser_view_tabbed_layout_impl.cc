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
#include "brave/browser/ui/views/tabs/vertical_tab_utils.h"
#include "build/build_config.h"
#include "chrome/browser/profiles/profile.h"
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

  // In focus mode, the horizontal tab strip is reparented into the top
  // container. Since the tab strip does not paint its own background, the top
  // container background must be set to the frame color instead of the toolbar
  // background color.
  if (IsParentedTo(views().horizontal_tab_strip_region_view,
                   views().top_container)) {
    if (!delegate().ShouldShowVerticalTabs()) {
      background->SetPrimaryColor(ui::kColorFrameActive);
    }
  }
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
    const gfx::Rect& panel_bounds) {
  // The upstream layout animates the panel by varying panel.x:
  //   trailing panel: x = right_edge - visible_width (slides in from high X)
  //   leading  panel: x = left_edge - (target - visible_width) (from low X)
  //
  // Shifting by ±sidebar_width offsets the whole slide range without changing
  // the animation value, so the panel animates correctly against the sidebar
  // edge instead of the browser edge. The sidebar and the upstream panel share
  // the alignment pref, so `sidebar_leading` matches upstream's
  // `side_panel_leading` and the shift direction agrees with where upstream
  // placed the panel.
  gfx::Rect result = panel_bounds;
  if (sidebar_leading) {
    // Sidebar at the leading edge: shift panel toward high X.
    result.set_x(panel_bounds.x() + sidebar_bounds.width());
  } else {
    // Sidebar at the trailing edge: shift panel toward low X.
    result.set_x(panel_bounds.x() - sidebar_bounds.width());
  }
  return result;
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
    const BrowserLayoutParams& browser_params) const {
  BrowserLayoutParams params = browser_params;
  if (auto title_bar = views().focus_mode_title_bar) {
    if (title_bar->GetVisible()) {
      params.SetTop(title_bar->GetPreferredSize().height());
    }
  }

  // Get the base layout from parent class
  ProposedLayout layout =
      BrowserViewTabbedLayoutImpl::CalculateProposedLayout(params);

  if (auto title_bar = views().focus_mode_title_bar) {
    gfx::Rect title_bar_bounds;
    if (title_bar->GetVisible()) {
      auto& client_area = browser_params.visual_client_area;
      title_bar_bounds =
          gfx::Rect(client_area.x(), client_area.y(), client_area.width(),
                    title_bar->GetPreferredSize().height());
    }
    layout.AddChild(title_bar, title_bar_bounds);
  }

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

  // Adjust infobar layout if vertical tabs are shown. i.e. sets insets to
  // infobar_container considering vertical tab strip. On macOS, the insets can
  // have bottom insets but it doesn't need for info bar.
  if (views().vertical_tab_strip_host && delegate().IsInfobarVisible()) {
    auto* infobar_layout = layout.GetLayoutFor(views().infobar_container);
    CHECK(infobar_layout);
    if (infobar_layout && infobar_layout->visibility.value_or(true)) {
      gfx::Insets insets = GetInsetsConsideringVerticalTabHost();
      insets.set_bottom(0);
      infobar_layout->bounds.Inset(insets);
    }
  }

  return layout;
}

gfx::Rect BraveBrowserViewTabbedLayoutImpl::CalculateTopContainerLayoutImpl(
    ProposedLayout& layout,
    BrowserLayoutParams params,
    bool needs_exclusion,
    bool suppress_top_separator) const {
  // Upstream suppresses the top separator when the side panel is shown and
  // GetTopSeparatorType() == kTopContainer. Brave always wants the separator
  // visible in that case, so undo only that specific suppression.
  // See suppress_top_separator var in
  // BrowserViewTabbedLayoutImpl::CalculateProposedLayout().
  if (GetTopSeparatorType() == TopSeparatorType::kTopContainer) {
    suppress_top_separator = false;
  }

  // Get base layout from parent
  gfx::Rect bounds =
      BrowserViewTabbedLayoutImpl::CalculateTopContainerLayoutImpl(
          layout, params, needs_exclusion, suppress_top_separator);

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
    // In focus mode the top chrome slides over the contents area rather than
    // pushing it down. Anchor the vertical tab strip to the top of the
    // contents bounds so it stays full-height and the revealed top views
    // overlay it.
    if (!IsParentedTo(views().top_container, views().browser_view)) {
      auto* contents_layout = layout.GetLayoutFor(views().contents_container);
      CHECK(contents_layout);
      return contents_layout->bounds.y();
    }

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
    if (auto* top_layout = layout.GetLayoutFor(views().top_container)) {
      return top_layout->bounds.bottom() - GetContentsMargins().top();
    }

    return GetContentsMargins().top();
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
  // In V2, sidebar_container holds only the control view and the upstream
  // toolbar_height_side_panel is a direct child of browser_view positioned
  // separately.  In V1, sidebar_container wraps both the control and the side
  // panel, and the toolbar_height_side_panel pointer is NOT inside the
  // container (so layout.GetLayoutFor returns null). The adjust_panel lambda
  // in this function is a no-op in V1 and meaningful only in V2.

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
  // sidebar (contents_bounds.width()).  In V1 this is the full available width
  // minus the vtab; in V2 it is further reduced by the upstream side panel
  // width, but the sidebar control is narrow enough that the cap never fires.
  const int sidebar_width = GetIdealSideBarWidth(contents_bounds.width());

  const gfx::Rect sidebar_bounds = ComputeSidebarBounds(
      sidebar_leading, sidebar_width, outer_left, outer_right,
      contents_bounds.y(), contents_bounds.height());

  // Shift upstream side panels (V2 only; no-op in V1 since those panels are
  // inside sidebar_container and are not top-level layout entries) inward so
  // they sit between the contents and the sidebar control.
  auto adjust_panel = [&](SidePanel* panel) {
    if (!panel) {
      return;
    }
    auto* panel_layout = layout.GetLayoutFor(panel);
    if (!panel_layout) {
      return;
    }
    panel_layout->bounds = ComputeAdjustedPanelBounds(
        sidebar_leading, sidebar_bounds, panel_layout->bounds);
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

#if BUILDFLAG(IS_MAC)
  insets = AddVerticalTabFrameBorderInsets(insets);
#endif

  views().vertical_tab_strip_host->SetBorder(
      insets.IsEmpty() ? nullptr : views::CreateEmptyBorder(insets));
}

bool BraveBrowserViewTabbedLayoutImpl::ShadowOverlayVisible() const {
  // Brave manages its own rounded-corners shadow around the contents and side
  // panel via BraveContentsViewUtil. Suppress the upstream shadow overlay (and
  // its accompanying main-area padding) so it doesn't double up.
  return false;
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

  gfx::Insets margins(kRoundedCornersContentsViewMargin);

  auto contents_at_top_edge = [&]() {
    if (delegate().IsInfobarVisible()) {
      return false;
    }
    if (auto focus_mode_title_bar = views().focus_mode_title_bar) {
      if (focus_mode_title_bar->GetVisible()) {
        return false;
      }
    }
    // In focus mode the top container is reparented out of the browser view, so
    // the top chrome no longer pushes the contents down. Only treat top UI as
    // occupying the top edge when the top container is still a child of the
    // browser view.
    if (IsParentedTo(views().top_container, views().browser_view)) {
      if (delegate().ShouldDrawTabStrip() || delegate().IsToolbarVisible() ||
          delegate().IsBookmarkBarVisible()) {
        return false;
      }
    }
    return true;
  };

  if (!contents_at_top_edge()) {
    margins.set_top(0);
  }

  return margins;
}

bool BraveBrowserViewTabbedLayoutImpl::ShouldPushBookmarkBarForVerticalTabs()
    const {
  CHECK(views().vertical_tab_strip_host)
      << "This method is used only when vertical tab strip host is set";

  // In focus mode, the top container (and the bookmark view within it) has been
  // parented to the top overlay and the bookmark bar does not need to be
  // repositioned.
  if (!IsParentedTo(views().top_container, views().browser_view)) {
    return false;
  }

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
