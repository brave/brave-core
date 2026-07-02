/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/frame/brave_contents_view_util.h"

#include "base/check.h"
#include "brave/browser/ui/views/frame/brave_browser_view.h"
#include "brave/browser/ui/views/frame/vertical_tabs/vertical_tab_strip_container_view.h"
#include "brave/browser/ui/views/frame/vertical_tabs/vertical_tab_strip_region_view.h"
#include "brave/browser/ui/views/tabs/vertical_tab_utils.h"
#include "brave/ui/color/nala/nala_color_id.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser_window/public/browser_window_interface.h"
#include "chrome/browser/ui/color/chrome_color_id.h"
#include "chrome/browser/ui/layout_constants.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "chrome/browser/ui/views/side_panel/side_panel.h"
#include "chrome/common/pref_names.h"
#include "components/prefs/pref_service.h"
#include "components/split_tabs/split_tab_id.h"
#include "components/tabs/public/split_tab_data.h"
#include "ui/color/color_provider.h"
#include "ui/gfx/geometry/insets.h"
#include "ui/gfx/geometry/rounded_corners_f.h"
#include "ui/views/border.h"
#include "ui/views/layout/layout_provider.h"

using views::ShapeContextTokensOverride::kRoundedCornersBorderRadius;
using views::ShapeContextTokensOverride::
    kRoundedCornersBorderRadiusAtWindowCorner;

// static
std::unique_ptr<views::Border>
BraveContentsViewUtil::CreateContentsOutlineBorder(
    const ui::ColorProvider* color_provider,
    const gfx::RoundedCornersF& corner_radii) {
  CHECK(color_provider);
  return views::CreateBorderPainter(
      views::Painter::CreateRoundRectWith1PxBorderPainter(
          color_provider->GetColor(kColorToolbar),
          color_provider->GetColor(
              nala::kColorDesktopbrowserToolbarButtonOutline),
          corner_radii, SkBlendMode::kSrc,
          /*antialias=*/true,
          /*should_border_scale=*/true),
      gfx::Insets(kRoundedCornersContentsOutlineThickness));
}

// static
int BraveContentsViewUtil::GetRoundedCornersWebViewMargin(Browser* browser) {
  return BraveBrowserView::ShouldUseBraveWebViewRoundedCornersForContents(
             browser)
             ? kRoundedCornersContentsViewMargin
             : 0;
}

// static
int BraveContentsViewUtil::GetRoundedCornersWebViewMargin(
    const Browser* browser) {
  return BraveBrowserView::ShouldUseBraveWebViewRoundedCornersForContents(
             browser)
             ? kRoundedCornersContentsViewMargin
             : 0;
}

// static
gfx::RoundedCornersF BraveContentsViewUtil::GetRoundedCornersForContentsView(
    BrowserWindowInterface* browser_window_interface,
    tabs::TabInterface* tab) {
  auto* layout_provider = views::LayoutProvider::Get();
  auto rounded_corners =
      gfx::RoundedCornersF(layout_provider->GetCornerRadiusMetric(
          kRoundedCornersBorderRadiusAtWindowCorner));
  const auto rounded_corners_border_radius =
      layout_provider->GetCornerRadiusMetric(kRoundedCornersBorderRadius);
  rounded_corners.set_upper_left(rounded_corners_border_radius);
  rounded_corners.set_upper_right(rounded_corners_border_radius);

  auto* browser_view = BraveBrowserView::From(
      BrowserView::GetBrowserViewForBrowser(browser_window_interface));

  // Can null during the startup.
  if (!browser_view) {
    return rounded_corners;
  }

  bool show_vertical_tab = tabs::utils::ShouldShowBraveVerticalTabs(
      browser_window_interface->GetBrowserForMigrationOnly());
  auto* vertical_tab_strip_container_view =
      browser_view->vertical_tab_strip_container_view();

  // When hide completely is on, we think vertical tab is invisible
  // except it's expanded.
  if (show_vertical_tab && vertical_tab_strip_container_view) {
    auto* vtsr_view =
        vertical_tab_strip_container_view->vertical_tab_strip_region_view();
    CHECK(vtsr_view);
    if (tabs::utils::ShouldHideVerticalTabsCompletelyWhenCollapsed(
            browser_window_interface->GetBrowserForMigrationOnly())) {
      show_vertical_tab = (vtsr_view->state() ==
                           BraveVerticalTabStripRegionView::State::kExpanded);
    }
  }

  // Check there is another ui between contents view and browser window border.
  // It affects contents view's lower-left/right radius.
  bool has_left_side_ui = false;
  bool has_right_side_ui = false;

  if (show_vertical_tab) {
    if (tabs::utils::IsVerticalTabOnRight(
            browser_window_interface->GetBrowserForMigrationOnly())) {
      has_right_side_ui = true;
    } else {
      has_left_side_ui = true;
    }
  }

  // Checking the sidebar UI alone is not sufficient because the panel can be
  // visible on its own.
  // TODO(https://github.com/brave/brave-browser/issues/56248): Rethink the
  // naming convention. Some names are inaccurate: the panel is separated from
  // the container view, so IsSidebarVisible() reflects only the sidebar control
  // UI and not a panel that is visible alone.
  if (browser_view->IsSidebarVisible() ||
      (browser_view->side_panel() &&
       browser_view->side_panel()->GetVisible())) {
    if (browser_window_interface->GetProfile()->GetPrefs()->GetBoolean(
            prefs::kSidePanelHorizontalAlignment)) {
      has_right_side_ui = true;
    } else {
      has_left_side_ui = true;
    }
  }

  if (tab && tab->IsSplit()) {
    auto split_tab_id = tab->GetSplit();
    auto* tab_strip_model = browser_window_interface->GetTabStripModel();
    auto* split_data = tab_strip_model->GetSplitData(*split_tab_id);

    // Handle each split tab's lower edge.
    if (split_data->ListTabs()[0] == tab) {
      has_right_side_ui = true;
    } else {
      has_left_side_ui = true;
    }
  }

  if (has_right_side_ui) {
    rounded_corners.set_lower_right(rounded_corners_border_radius);
  }

  if (has_left_side_ui) {
    rounded_corners.set_lower_left(rounded_corners_border_radius);
  }

  return rounded_corners;
}
