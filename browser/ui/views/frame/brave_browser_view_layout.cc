// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#include "brave/browser/ui/views/frame/brave_browser_view_layout.h"

#include <algorithm>
#include <limits>
#include <utility>

#include "brave/browser/ui/brave_browser.h"
#include "brave/browser/ui/views/frame/brave_contents_view_util.h"
#include "brave/browser/ui/views/sidebar/sidebar_container_view.h"
#include "brave/browser/ui/views/tabs/vertical_tab_utils.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_window/public/browser_window_features.h"
#include "chrome/browser/ui/exclusive_access/exclusive_access_manager.h"
#include "chrome/browser/ui/exclusive_access/fullscreen_controller.h"
#include "chrome/browser/ui/views/bookmarks/bookmark_bar_view.h"
#include "chrome/browser/ui/views/frame/browser_view.h"
#include "chrome/browser/ui/views/frame/browser_view_layout_delegate.h"
#include "chrome/browser/ui/views/infobars/infobar_container_view.h"
#include "chrome/browser/ui/views/side_panel/side_panel_ui.h"
#include "components/bookmarks/common/bookmark_pref_names.h"
#include "ui/views/border.h"
#include "ui/views/view_class_properties.h"

namespace {

constexpr int kSidebarSeparatorWidth = 1;
constexpr int kSidebarSeparatorMargin = 4;

}  // namespace

BraveBrowserViewLayout::BraveBrowserViewLayout(
    std::unique_ptr<BrowserViewLayoutDelegate> delegate,
    BrowserView* browser_view,
    views::View* top_container,
    WebAppFrameToolbarView* web_app_frame_toolbar,
    views::Label* web_app_window_title,
    TabStripRegionView* tab_strip_region_view,
    TabStrip* tab_strip,
    views::View* toolbar,
    InfoBarContainerView* infobar_container,
    views::View* contents_container,
    views::View* left_aligned_side_panel_separator,
    views::View* unified_side_panel,
    views::View* right_aligned_side_panel_separator,
    views::View* side_panel_rounded_corner,
    ImmersiveModeController* immersive_mode_controller,
    views::View* contents_separator)
    : BrowserViewLayout(
          std::move(delegate),
          browser_view,
          top_container,
          web_app_frame_toolbar,
          web_app_window_title,
          tab_strip_region_view,
          tab_strip,
          toolbar,
          infobar_container,
          (browser_view ? browser_view->GetContentsContainerForLayoutManager()
                        : browser_view),
          left_aligned_side_panel_separator,
          unified_side_panel,
          right_aligned_side_panel_separator,
          side_panel_rounded_corner,
          immersive_mode_controller,
          contents_separator) {}

BraveBrowserViewLayout::~BraveBrowserViewLayout() = default;

int BraveBrowserViewLayout::GetIdealSideBarWidth() const {
  if (!sidebar_container_) {
    return 0;
  }

  return GetIdealSideBarWidth(contents_container_->width() +
                              GetContentsMargins().width() +
                              sidebar_container_->width());
}

int BraveBrowserViewLayout::GetIdealSideBarWidth(int available_width) const {
  if (!sidebar_container_) {
    return 0;
  }

  int sidebar_width = sidebar_container_->GetPreferredSize().width();

  // The sidebar can take up the entire space for fullscreen.
  if (sidebar_width == std::numeric_limits<int>::max()) {
    return available_width;
  }

  // The sidebar should take up no more than 80% of the content area.
  return std::min<int>(available_width * 0.8, sidebar_width);
}

void BraveBrowserViewLayout::Layout(views::View* host) {
  BrowserViewLayout::Layout(host);
  LayoutVerticalTabs();
}

void BraveBrowserViewLayout::LayoutVerticalTabs() {
  if (!vertical_tab_strip_host_.get()) {
    return;
  }

  if (!tabs::utils::ShouldShowVerticalTabs(browser_view_->browser())) {
    vertical_tab_strip_host_->SetBorder(nullptr);
    vertical_tab_strip_host_->SetBoundsRect({});
    return;
  }

  auto get_vertical_tabs_top = [&]() {
    if (ShouldPushBookmarkBarForVerticalTabs()) {
      return bookmark_bar_->y();
    }
    if (IsInfobarVisible()) {
      return infobar_container_->y();
    }
    if (IsReaderModeToolbarVisible()) {
      return reader_mode_toolbar_->y();
    }
    return contents_container_->y() - GetContentsMargins().top();
  };

  gfx::Rect vertical_tab_strip_bounds = vertical_layout_rect_;
  vertical_tab_strip_bounds.SetVerticalBounds(get_vertical_tabs_top(),
                                              browser_view_->height());
  gfx::Insets insets;
#if !BUILDFLAG(IS_LINUX)
  // When the bookmark bar is adjacent to the tabstrip, the separator between
  // the bookmark bar and the content area will also be adjacent, instead of
  // above. In order to avoid tabstrip position changes when switching to a
  // different tab, add some spacing as if the separator were above and
  // invisible.
  if (contents_separator_ && ShouldPushBookmarkBarForVerticalTabs()) {
    insets.set_top(contents_separator_->GetPreferredSize().height());
  }
#endif  // BUILDFLAG(IS_LINUX)

#if BUILDFLAG(IS_MAC)
  insets = AdjustInsetsConsideringFrameBorder(insets);
#endif

  if (insets.IsEmpty()) {
    vertical_tab_strip_host_->SetBorder(nullptr);
  } else {
    vertical_tab_strip_host_->SetBorder(views::CreateEmptyBorder(insets));
  }

  const auto width =
      vertical_tab_strip_host_->GetPreferredSize().width() + insets.width();
  if (tabs::utils::IsVerticalTabOnRight(browser_view_->browser())) {
    vertical_tab_strip_bounds.set_x(vertical_tab_strip_bounds.right() - width);
  }
  vertical_tab_strip_bounds.set_width(width);
  vertical_tab_strip_host_->SetBoundsRect(vertical_tab_strip_bounds);
}

void BraveBrowserViewLayout::LayoutSidePanelView(
    views::View* side_panel,
    gfx::Rect& contents_container_bounds) {
  if (contents_background_) {
    contents_background_->SetBoundsRect(contents_container_bounds);
  }

  LayoutSideBar(contents_container_bounds);
  LayoutReaderModeToolbar(contents_container_bounds);

  UpdateContentsContainerInsets(contents_container_bounds);
}

int BraveBrowserViewLayout::LayoutTabStripRegion(int top) {
  if (tabs::utils::ShouldShowVerticalTabs(browser_view_->browser())) {
    // In case we're using vertical tabstrip, we can decide the position
    // after we finish laying out views in top container.
    return top;
  }

  return BrowserViewLayout::LayoutTabStripRegion(top);
}

int BraveBrowserViewLayout::LayoutBookmarkAndInfoBars(int top,
                                                      int browser_view_y) {
  if (!vertical_tab_strip_host_ || !ShouldPushBookmarkBarForVerticalTabs()) {
    return BrowserViewLayout::LayoutBookmarkAndInfoBars(top, browser_view_y);
  }

  auto new_rect = vertical_layout_rect_;
  new_rect.Inset(GetInsetsConsideringVerticalTabHost());
  base::AutoReset resetter(&vertical_layout_rect_, new_rect);
  return BrowserViewLayout::LayoutBookmarkAndInfoBars(top, browser_view_y);
}

int BraveBrowserViewLayout::LayoutInfoBar(int top) {
  if (!vertical_tab_strip_host_) {
    return BrowserViewLayout::LayoutInfoBar(top);
  }

  if (ShouldPushBookmarkBarForVerticalTabs()) {
    // Insets are already applied from LayoutBookmarkAndInfoBar().
    return BrowserViewLayout::LayoutInfoBar(top);
  }

  auto new_rect = vertical_layout_rect_;
  new_rect.Inset(GetInsetsConsideringVerticalTabHost());
  base::AutoReset resetter(&vertical_layout_rect_, new_rect);
  return BrowserViewLayout::LayoutInfoBar(top);
}

void BraveBrowserViewLayout::LayoutContentsContainerView(int top, int bottom) {
  if (!vertical_tab_strip_host_) {
    BrowserViewLayout::LayoutContentsContainerView(top, bottom);
    return;
  }

  auto new_rect = vertical_layout_rect_;
  new_rect.Inset(GetInsetsConsideringVerticalTabHost());
  base::AutoReset resetter(&vertical_layout_rect_, new_rect);
  return BrowserViewLayout::LayoutContentsContainerView(top, bottom);
}

void BraveBrowserViewLayout::LayoutSideBar(gfx::Rect& contents_bounds) {
  if (!sidebar_container_) {
    return;
  }

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
  const bool on_left = sidebar_container_->sidebar_on_left();
  if (on_left) {
    contents_bounds.set_x(contents_bounds.x() + sidebar_bounds.width());

    // When vertical tabs and the sidebar are adjacent, add a separator between
    // them.
    if (tabs::utils::ShouldShowVerticalTabs(browser_view_->browser()) &&
        sidebar_separator_ && !sidebar_bounds.IsEmpty()) {
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

  gfx::Insets panel_margins = GetContentsMargins();
  if (BraveBrowser::ShouldUseBraveWebViewRoundedCorners(
          browser_view_->browser())) {
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
  sidebar_container_->side_panel()->SetProperty(views::kMarginsKey,
                                                panel_margins);

  sidebar_container_->SetBoundsRect(
      browser_view_->GetMirroredRect(sidebar_bounds));

  if (sidebar_separator_) {
    if (separator_bounds.IsEmpty()) {
      sidebar_separator_->SetVisible(false);
    } else {
      sidebar_separator_->SetBoundsRect(
          browser_view_->GetMirroredRect(separator_bounds));
      sidebar_separator_->SetVisible(true);
    }
  }
}

void BraveBrowserViewLayout::UpdateContentsContainerInsets(
    gfx::Rect& contents_container_bounds) {
  // Control contents's margin with sidebar & vertical tab state.
  gfx::Insets contents_margins = GetContentsMargins();

  // Don't need to have additional contents margin for rounded corners
  // in tab-initiated fullscreen. Web contents occupies whole screen.
  if (IsFullscreenForTab()) {
    contents_container_bounds.Inset(contents_margins);
    return;
  }

  // In rounded corners mode, we need to include a little margin so we have
  // somewhere to draw the shadow.
  int contents_margin_for_rounded_corners =
      BraveContentsViewUtil::GetRoundedCornersWebViewMargin(
          browser_view_->browser());

  // Don't need contents container's left or right margin with vertical tab as
  // vertical tab itself has sufficient padding.
  if (tabs::utils::ShouldShowVerticalTabs(browser_view_->browser()) &&
      !IsFullscreenForBrowser()) {
    if (tabs::utils::IsVerticalTabOnRight(browser_view_->browser())) {
      contents_margins.set_right(contents_margin_for_rounded_corners);
    } else {
      contents_margins.set_left(contents_margin_for_rounded_corners);
    }
  }

  // If side panel is shown, contents container should have margin
  // because panel doesn't have margin.
  if (browser_view_->browser()
          ->GetFeatures()
          .side_panel_ui()
          ->GetCurrentEntryId()) {
    contents_container_bounds.Inset(contents_margins);
    return;
  }

  // If sidebar UI is not shown, contents container should have margin.
  if (!sidebar_container_ || !sidebar_container_->IsSidebarVisible()) {
    contents_container_bounds.Inset(contents_margins);
    return;
  }

  // If sidebar UI is only shown, contents container should have margin
  // based on sidebar's position because sidebar UI itself has padding always.
  // If sidebar is shown in left-side, contents container doens't need its
  // left margin.
  if (sidebar_container_->sidebar_on_left()) {
    contents_margins.set_left(contents_margin_for_rounded_corners);
  } else {
    contents_margins.set_right(contents_margin_for_rounded_corners);
  }
  contents_container_bounds.Inset(contents_margins);
}

void BraveBrowserViewLayout::LayoutReaderModeToolbar(
    gfx::Rect& contents_bounds) {
  if (!IsReaderModeToolbarVisible()) {
    return;
  }

  gfx::Rect bounds = contents_bounds;
  bounds.set_height(reader_mode_toolbar_->GetPreferredSize().height());
  reader_mode_toolbar_->SetBoundsRect(bounds);

  contents_bounds.Inset(gfx::Insets::TLBR(bounds.height(), 0, 0, 0));
}

gfx::Insets BraveBrowserViewLayout::GetContentsMargins() const {
  if (!BraveBrowser::ShouldUseBraveWebViewRoundedCorners(
          browser_view_->browser())) {
    return {};
  }

  if (IsFullscreenForTab()) {
    return {};
  }

  gfx::Insets margins(BraveContentsViewUtil::kMarginThickness);

  // If there is a visible view above the contents container, then there is no
  // no need for a top margin.
  if (browser_view_->GetTabStripVisible() ||
      browser_view_->IsToolbarVisible() ||
      browser_view_->IsBookmarkBarVisible() || IsInfobarVisible() ||
      IsReaderModeToolbarVisible()) {
    margins.set_top(0);
  }

  return margins;
}

bool BraveBrowserViewLayout::IsReaderModeToolbarVisible() const {
  return reader_mode_toolbar_ && reader_mode_toolbar_->GetVisible();
}

bool BraveBrowserViewLayout::IsFullscreenForBrowser() const {
  auto* exclusive_access_manager = browser_view_->GetExclusiveAccessManager();
  if (!exclusive_access_manager) {
    return false;
  }
  auto* fullscreen_controller =
      exclusive_access_manager->fullscreen_controller();
  return fullscreen_controller &&
         fullscreen_controller->IsFullscreenForBrowser();
}

bool BraveBrowserViewLayout::IsFullscreenForTab() const {
  auto* exclusive_access_manager = browser_view_->GetExclusiveAccessManager();
  if (!exclusive_access_manager) {
    return false;
  }
  auto* fullscreen_controller =
      exclusive_access_manager->fullscreen_controller();
  return fullscreen_controller &&
         fullscreen_controller->IsWindowFullscreenForTabOrPending();
}

bool BraveBrowserViewLayout::ShouldPushBookmarkBarForVerticalTabs() {
  CHECK(vertical_tab_strip_host_)
      << "This method is used only when vertical tab strip host is set";

  // This can happen when bookmarks bar is visible on NTP. In this case
  // we should lay out vertical tab strip next to bookmarks bar so that
  // the tab strip doesn't move when changing the active tab.
  return bookmark_bar_ &&
         !browser_view_->browser()->profile()->GetPrefs()->GetBoolean(
             bookmarks::prefs::kShowBookmarkBar) &&
         delegate_->IsBookmarkBarVisible();
}

gfx::Insets BraveBrowserViewLayout::GetInsetsConsideringVerticalTabHost() {
  CHECK(vertical_tab_strip_host_)
      << "This method is used only when vertical tab strip host is set";
  gfx::Insets insets;
  if (tabs::utils::IsVerticalTabOnRight(browser_view_->browser())) {
    insets.set_right(vertical_tab_strip_host_->GetPreferredSize().width());
  } else {
    insets.set_left(vertical_tab_strip_host_->GetPreferredSize().width());
  }
#if BUILDFLAG(IS_MAC)
  insets = AdjustInsetsConsideringFrameBorder(insets);
#endif

  return insets;
}

#if BUILDFLAG(IS_MAC)
gfx::Insets BraveBrowserViewLayout::AdjustInsetsConsideringFrameBorder(
    const gfx::Insets& insets) {
  if (!tabs::utils::ShouldShowVerticalTabs(browser_view_->browser()) ||
      browser_view_->IsFullscreen()) {
    return insets;
  }

  // for frame border drawn by OS. Vertical tabstrip's widget shouldn't cover
  // that line
  auto new_insets(insets);
  if (tabs::utils::IsVerticalTabOnRight(browser_view_->browser())) {
    new_insets.set_right(1 + insets.right());
  } else {
    new_insets.set_left(1 + insets.left());
  }
  return new_insets;
}
#endif
