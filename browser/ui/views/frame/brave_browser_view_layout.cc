// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#include "brave/browser/ui/views/frame/brave_browser_view_layout.h"

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
    Browser* browser,
    BrowserViewLayoutViews views)
    : BrowserViewLayoutImplOld(std::move(delegate), browser, std::move(views)) {
  // TODO(anyone): consider a possibility of not passing the browser ptr in
  // since upstream is actively working on removing the browser from being
  // passed around. https://github.com/brave/brave-browser/issues/50815
  if (!browser) {
    CHECK_IS_TEST();
  }
}

BraveBrowserViewLayout::~BraveBrowserViewLayout() = default;

int BraveBrowserViewLayout::GetIdealSideBarWidth() const {
  if (!sidebar_container_) {
    return 0;
  }

  return GetIdealSideBarWidth(views().contents_container->width() +
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
  BrowserViewLayoutImplOld::Layout(host);
  LayoutVerticalTabs();
}

void BraveBrowserViewLayout::LayoutVerticalTabs() {
  if (!vertical_tab_strip_host_.get()) {
    return;
  }

  if (!browser() || !tabs::utils::ShouldShowVerticalTabs(browser())) {
    vertical_tab_strip_host_->SetBorder(nullptr);
    vertical_tab_strip_host_->SetBoundsRect({});
    return;
  }

  auto get_vertical_tabs_top = [&]() {
    if (ShouldPushBookmarkBarForVerticalTabs()) {
      return views().bookmark_bar->y();
    }
    if (IsInfobarVisible()) {
      return views().infobar_container->y();
    }
    return views().top_container->bounds().bottom() -
           GetContentsMargins().top();
  };

  gfx::Rect vertical_tab_strip_bounds = views().browser_view->GetLocalBounds();
  vertical_tab_strip_bounds.SetVerticalBounds(get_vertical_tabs_top(),
                                              views().browser_view->height());
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
  insets = AdjustInsetsConsideringFrameBorder(insets);
#endif

  if (insets.IsEmpty()) {
    vertical_tab_strip_host_->SetBorder(nullptr);
  } else {
    vertical_tab_strip_host_->SetBorder(views::CreateEmptyBorder(insets));
  }

  const auto width =
      vertical_tab_strip_host_->GetPreferredSize().width() + insets.width();
  if (browser() && tabs::utils::IsVerticalTabOnRight(browser())) {
    vertical_tab_strip_bounds.set_x(vertical_tab_strip_bounds.right() - width);
  }
  vertical_tab_strip_bounds.set_width(width);
  vertical_tab_strip_host_->SetBoundsRect(vertical_tab_strip_bounds);
}

void BraveBrowserViewLayout::LayoutTabStripRegion(gfx::Rect& available_bounds) {
  if (browser() && tabs::utils::ShouldShowVerticalTabs(browser())) {
    // In case we're using vertical tabstrip, we can decide the position
    // after we finish laying out views in top container.
    return;
  }

  return BrowserViewLayoutImplOld::LayoutTabStripRegion(available_bounds);
}

void BraveBrowserViewLayout::LayoutBookmarkBar(gfx::Rect& available_bounds) {
  if (!vertical_tab_strip_host_ || !ShouldPushBookmarkBarForVerticalTabs()) {
    BrowserViewLayoutImplOld::LayoutBookmarkBar(available_bounds);
    return;
  }

  // Set insets for vertical tab and restore after finishing infobar layout.
  // Each control's layout will consider vertical tab.
  const auto insets_for_vertical_tab = GetInsetsConsideringVerticalTabHost();
  available_bounds.Inset(insets_for_vertical_tab);
  BrowserViewLayoutImplOld::LayoutBookmarkBar(available_bounds);
  gfx::Outsets outsets_for_restore_insets;
  outsets_for_restore_insets.set_left_right(insets_for_vertical_tab.left(),
                                            insets_for_vertical_tab.right());
  available_bounds.Outset(outsets_for_restore_insets);
}

void BraveBrowserViewLayout::LayoutInfoBar(gfx::Rect& available_bounds) {
  if (!vertical_tab_strip_host_) {
    BrowserViewLayoutImplOld::LayoutInfoBar(available_bounds);
    return;
  }

  // Set insets for vertical tab and restore after finishing infobar layout.
  // Each control's layout will consider vertical tab.
  const auto insets_for_vertical_tab = GetInsetsConsideringVerticalTabHost();
  available_bounds.Inset(insets_for_vertical_tab);
  BrowserViewLayoutImplOld::LayoutInfoBar(available_bounds);
  gfx::Outsets outsets_for_restore_insets;
  outsets_for_restore_insets.set_left_right(insets_for_vertical_tab.left(),
                                            insets_for_vertical_tab.right());
  available_bounds.Outset(outsets_for_restore_insets);
}

void BraveBrowserViewLayout::LayoutContentsContainerView(
    const gfx::Rect& available_bounds) {
  if (contents_background_) {
    contents_background_->SetBoundsRect(available_bounds);
  }

  gfx::Rect contents_container_bounds = available_bounds;
  if (vertical_tab_strip_host_) {
    // Both vertical tab impls should not be enabled together.
    // https://github.com/brave/brave-browser/issues/48373
    CHECK(!tabs::IsVerticalTabsFeatureEnabled());
    contents_container_bounds.Inset(GetInsetsConsideringVerticalTabHost());
  }

  if (views().webui_tab_strip && views().webui_tab_strip->GetVisible()) {
    // The WebUI tab strip container should "push" the tab contents down without
    // resizing it.
    contents_container_bounds.Inset(
        gfx::Insets().set_bottom(-views().webui_tab_strip->size().height()));
  }

  LayoutSideBar(contents_container_bounds);
  UpdateContentsContainerInsets(contents_container_bounds);

  views().contents_container->SetBoundsRect(contents_container_bounds);
}

bool BraveBrowserViewLayout::IsImmersiveModeEnabledWithoutToolbar() const {
  // When return true here, BrowserViewLayout::LayoutBookmarkAndInfoBars()
  // makes |top_container_separator_| visible.
  // We want to use |top_container_separator_| as a separator between
  // top container and contents instead of MultiContentsContainer's top
  // separator to cover whole browser window width.
  // Althought it's always visible, it's only shown when rounded corners
  // is not applied by controlling its bounds from BraveBrowserView.
  return true;
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
    if (browser() && tabs::utils::ShouldShowVerticalTabs(browser()) &&
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
  if (browser() &&
      BraveBrowserView::ShouldUseBraveWebViewRoundedCornersForContents(
          browser())) {
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
      views().browser_view->GetMirroredRect(sidebar_bounds));

  if (sidebar_separator_) {
    if (separator_bounds.IsEmpty()) {
      sidebar_separator_->SetVisible(false);
    } else {
      sidebar_separator_->SetBoundsRect(
          views().browser_view->GetMirroredRect(separator_bounds));
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
      browser()
          ? BraveContentsViewUtil::GetRoundedCornersWebViewMargin(browser())
          : 0;

  // Due to vertical tab's padding(tabs::kMarginForVerticalTabContainers), we
  // can see some space between vertical tab and contents. However, If we don't
  // have margin from contents, vertical tab side contents shadow isn't visible.
  // So, having half of margin from vertical tab and half from contents.
  if (browser() && tabs::utils::ShouldShowVerticalTabs(browser()) &&
      (vertical_tab_strip_host_ &&
       vertical_tab_strip_host_->GetPreferredSize().width() != 0) &&
      !IsFullscreenForBrowser()) {
    const int margin_with_vertical_tab =
        browser()
            ? BraveBrowserView::ShouldUseBraveWebViewRoundedCornersForContents(
                  browser())
                  ? (tabs::kMarginForVerticalTabContainers / 2)
                  : 0
            : 0;
    if (browser() && tabs::utils::IsVerticalTabOnRight(browser())) {
      contents_margins.set_right(margin_with_vertical_tab);
    } else {
      contents_margins.set_left(margin_with_vertical_tab);
    }
  }

  // If side panel is shown, contents container should have margin
  // because panel doesn't have margin.
  if (browser() &&
      browser()->GetFeatures().side_panel_ui()->GetCurrentEntryId()) {
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

gfx::Insets BraveBrowserViewLayout::GetContentsMargins() const {
  if (!browser() ||
      !BraveBrowserView::ShouldUseBraveWebViewRoundedCornersForContents(
          browser())) {
    return {};
  }

  if (IsFullscreenForTab()) {
    return {};
  }

  gfx::Insets margins(BraveContentsViewUtil::kMarginThickness);

  // If there is a visible view above the contents container, then there is no
  // no need for a top margin.
  // TODO(https://github.com/brave/brave-browser/issues/50488): This particular
  // cast should not exist, and possibly some of these functions should be
  // relying on `BrowserViewLayoutDelegate` to retrieve the necessary
  // information.
  BrowserView* browser_view = static_cast<BrowserView*>(views().browser_view);
  if (browser_view->GetTabStripVisible() || browser_view->IsToolbarVisible() ||
      browser_view->IsBookmarkBarVisible() || IsInfobarVisible()) {
    margins.set_top(0);
  }

  return margins;
}

bool BraveBrowserViewLayout::IsFullscreenForBrowser() const {
  if (!browser()) {
    return false;
  }
  // TODO(https://github.com/brave/brave-browser/issues/50488): This direct
  // access to `browser_` should be corrected to be done through `browser()`
  // method, however this requires correcting some of the constness around the
  // data being accessed.
  ExclusiveAccessManager* exclusive_access_manager =
      browser_->GetFeatures().exclusive_access_manager();
  if (!exclusive_access_manager) {
    return false;
  }
  auto* fullscreen_controller =
      exclusive_access_manager->fullscreen_controller();
  return fullscreen_controller &&
         fullscreen_controller->IsFullscreenForBrowser();
}

bool BraveBrowserViewLayout::IsFullscreenForTab() const {
  // TODO(https://github.com/brave/brave-browser/issues/50488): This direct
  // access to `browser_` should be corrected to be done through `browser()`
  // method, however this requires correcting some of the constness around the
  // data being accessed.
  if (!browser()) {
    return false;
  }
  ExclusiveAccessManager* exclusive_access_manager =
      browser_->GetFeatures().exclusive_access_manager();
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
  return views().bookmark_bar && browser() &&
         !browser()->profile()->GetPrefs()->GetBoolean(
             bookmarks::prefs::kShowBookmarkBar) &&
         delegate_->IsBookmarkBarVisible();
}

gfx::Insets BraveBrowserViewLayout::GetInsetsConsideringVerticalTabHost() {
  CHECK(vertical_tab_strip_host_)
      << "This method is used only when vertical tab strip host is set";
  gfx::Insets insets;
  if (browser() && tabs::utils::IsVerticalTabOnRight(browser())) {
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
  BrowserView* browser_view = BrowserView::GetBrowserViewForBrowser(browser());
  if (!browser() || !tabs::utils::ShouldShowVerticalTabs(browser()) ||
      (vertical_tab_strip_host_ &&
       vertical_tab_strip_host_->GetPreferredSize().width() == 0) ||
      (browser_view && browser_view->IsFullscreen())) {
    return insets;
  }

  // for frame border drawn by OS. Vertical tabstrip's widget shouldn't cover
  // that line
  auto new_insets(insets);
  if (tabs::utils::IsVerticalTabOnRight(browser())) {
    new_insets.set_right(1 + insets.right());
  } else {
    new_insets.set_left(1 + insets.left());
  }
  return new_insets;
}
#endif
