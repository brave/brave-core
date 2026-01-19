// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

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

namespace {

constexpr int kSidebarSeparatorWidth = 1;
constexpr int kSidebarSeparatorMargin = 4;

}  // namespace

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
  // 1. Get the base layout from parent class
  ProposedLayout layout =
      BrowserViewTabbedLayoutImpl::CalculateProposedLayout(params);

  // 2. Retrieve contents container proposed bounds.
  auto* contents_layout = layout.GetLayoutFor(views().contents_container);
  CHECK(contents_layout);

  // 5. Handle contents background - contents background should be laid out
  // before other views like sidebar or vertical tab strip in order to cover the
  // entire contents area that contains sidebar. Otherwise, we would have hole
  // between contents background and sidebar when using rounded corners.
  if (views().contents_background && contents_layout) {
    layout.AddChild(views().contents_background, contents_layout->bounds);
  }

  // 3. Apply vertical tab strip insets for contents container BEFORE laying out
  // sidebar, so the sidebar is positioned adjacent to (not underneath) the
  // vertical tab strip when it's on the right. This is because sidebar is
  // laid out depending on the contents_layout->bounds.
  if (views().vertical_tab_strip_host && delegate().ShouldShowVerticalTabs()) {
    // Both vertical tab impls should not be enabled together.
    CHECK(!tabs::IsVerticalTabsFeatureEnabled());
    contents_layout->bounds.Inset(GetInsetsConsideringVerticalTabHost());
  }

  if (views().webui_tab_strip && views().webui_tab_strip->GetVisible()) {
    // The WebUI tab strip container should "push" the tab contents down without
    // resizing it.
    contents_layout->bounds.Inset(
        gfx::Insets().set_bottom(-views().webui_tab_strip->size().height()));
  }

  // 4. Handle sidebar and adjust contents container bounds. This should be done
  // BEFORE calling `InsetContentsContainerBounds()` so that the contents
  // container's final bounds is updated considering the sidebar's bounds.
  CalculateSideBarLayout(layout, params);

  // 6. Update contents container bounds considering other views like sidebar
  // and vertical tab strip. when the other views are visible, contents
  // container's final bounds will be smaller than the original bounds.
  InsetContentsContainerBounds(layout);

  // 7. Proposed layout for the Brave vertical tab strip host.
  if (delegate().ShouldShowVerticalTabs()) {
    CalculateBraveVerticalTabStripLayout(layout, params);
  } else if (views().vertical_tab_strip_host) {
    // This is Brave specific view so the layout shouldn't be populated by
    // upstream's logic.
    CHECK(!layout.GetLayoutFor(views().vertical_tab_strip_host));
    layout.AddChild(views().vertical_tab_strip_host, gfx::Rect());
  }

  // etc. Adjust infobar layout if vertical tabs are shown. i.e. sets insets to
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

gfx::Rect BraveBrowserViewTabbedLayoutImpl::CalculateTopContainerLayout(
    ProposedLayout& layout,
    BrowserLayoutParams params,
    bool needs_exclusion) const {
  // 1. Get base layout from parent
  gfx::Rect bounds = BrowserViewTabbedLayoutImpl::CalculateTopContainerLayout(
      layout, params, needs_exclusion);

  if (!delegate().ShouldShowVerticalTabs()) {
    return bounds;
  }

  // 2. Adjust bookmark bar layout if vertical tabs are shown. i.e. sets insets
  // to bookmark_bar considering vertical tab strip. On macOS, the insets can
  // have bottom insets but it doesn't need for info bar.
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
}

BrowserViewTabbedLayoutImpl::TopSeparatorType
BraveBrowserViewTabbedLayoutImpl::GetTopSeparatorType() const {
  auto top_separator_type = BrowserViewTabbedLayoutImpl::GetTopSeparatorType();
  if (top_separator_type == TopSeparatorType::kNone) {
    // We always show top container separator.
    return TopSeparatorType::kTopContainer;
  }

  return top_separator_type;
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

  // Account for any additional frame-border insets on Mac.
  gfx::Insets insets;
#if BUILDFLAG(IS_MAC)
  insets = AddVerticalTabFrameBorderInsets(insets);
#endif

  const int width =
      views().vertical_tab_strip_host->GetPreferredSize().width() +
      insets.width();
  if (delegate().IsVerticalTabOnRight()) {
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

  gfx::Rect sidebar_bounds = contents_bounds;
  sidebar_bounds.set_width(GetIdealSideBarWidth(contents_bounds.width()));
  contents_bounds.set_width(contents_bounds.width() - sidebar_bounds.width());

#if BUILDFLAG(IS_MAC)
  // On Mac, setting an empty rect for the contents web view could cause a crash
  // in `StatusBubbleViews`. As the `StatusBubbleViews` width is one third of
  // the base view, set 3 here so that `StatusBubbleViews` can have a width of
  // at least 1.
  if (contents_bounds.width() <= 0) {
    contents_bounds.set_width(3);
  }
#endif

  gfx::Rect separator_bounds;
  const bool on_left = views().sidebar_container->sidebar_on_left();
  if (on_left) {
    contents_bounds.set_x(contents_bounds.x() + sidebar_bounds.width());

    // When vertical tabs and the sidebar are adjacent, add a separator between
    // them.
    if (delegate().ShouldShowVerticalTabs() && views().sidebar_separator &&
        !sidebar_bounds.IsEmpty()) {
      separator_bounds = sidebar_bounds;
      separator_bounds.set_width(kSidebarSeparatorWidth);
      separator_bounds.Inset(gfx::Insets::VH(kSidebarSeparatorMargin, 0));

      // Move sidebar and content over to make room for the separator.
      sidebar_bounds.set_x(sidebar_bounds.x() + kSidebarSeparatorWidth);
      contents_bounds.Inset(gfx::Insets::TLBR(0, kSidebarSeparatorWidth, 0, 0));
    }
  } else {
    sidebar_bounds.set_x(contents_bounds.right());
  }

  // Apply the updated contents bounds via ProposedLayout.
  contents_layout->bounds = contents_bounds;

  // Update sidebar bounds via ProposedLayout.
  const gfx::Rect mirrored_sidebar_bounds =
      views().browser_view->GetMirroredRect(sidebar_bounds);
  // This is Brave specific view so the layout shouldn't be populated by
  // upstream's logic.
  CHECK(views().sidebar_container);
  layout.AddChild(views().sidebar_container, mirrored_sidebar_bounds);

  // Sidebar separator bounds/visibility via ProposedLayout. The separator could
  // be null when IsBraveWebViewRoundedCornersEnabled() is false.
  if (views().sidebar_separator) {
    // This is Brave specific view so the layout shouldn't be populated by
    // upstream's logic.
    CHECK(!layout.GetLayoutFor(views().sidebar_separator));

    if (separator_bounds.IsEmpty()) {
      layout.AddChild(views().sidebar_separator, gfx::Rect(),
                      /*visibility=*/false);
    } else {
      layout.AddChild(views().sidebar_separator,
                      views().browser_view->GetMirroredRect(separator_bounds),
                      /*visibility=*/true);
    }
  }
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
  if (!views().sidebar_container ||
      !views().sidebar_container->IsSidebarVisible()) {
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
  views().sidebar_container->side_panel()->SetProperty(views::kMarginsKey,
                                                       panel_margins);
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
  // We need more care about frame border when vertical tab is visible.
  // Frame border is not drawn in fullscreen.
  if (!delegate().ShouldShowVerticalTabs() ||
      delegate().IsFullscreenForBrowser()) {
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
