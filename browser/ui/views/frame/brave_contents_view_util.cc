/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/frame/brave_contents_view_util.h"

#include "base/check.h"
#include "brave/browser/ui/views/frame/brave_browser_view.h"
#include "brave/browser/ui/views/frame/vertical_tabs/vertical_tab_strip_region_view.h"
#include "brave/browser/ui/views/frame/vertical_tabs/vertical_tab_strip_widget_delegate_view.h"
#include "brave/browser/ui/views/tabs/vertical_tab_utils.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser_window/public/browser_window_interface.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "chrome/common/pref_names.h"
#include "components/prefs/pref_service.h"
#include "components/tabs/public/split_tab_data.h"
#include "components/tabs/public/split_tab_id.h"
#include "ui/compositor/layer.h"
#include "ui/gfx/geometry/rounded_corners_f.h"
#include "ui/views/layout/layout_provider.h"
#include "ui/views/view.h"

using views::ShapeContextTokensOverride::kRoundedCornersBorderRadius;
using views::ShapeContextTokensOverride::
    kRoundedCornersBorderRadiusAtWindowCorner;

namespace {

constexpr ViewShadow::ShadowParameters kShadow{
    .offset_x = 0,
    .offset_y = 0,
    .blur_radius = BraveContentsViewUtil::kMarginThickness,
    .shadow_color = SkColorSetA(SK_ColorBLACK, 0.1 * 255)};
}  // namespace

std::unique_ptr<ViewShadow> BraveContentsViewUtil::CreateShadow(
    views::View* view) {
  DCHECK(view);
  auto* layout_provider = views::LayoutProvider::Get();
  auto shadow = std::make_unique<ViewShadow>(
      view, layout_provider->GetCornerRadiusMetric(kRoundedCornersBorderRadius),
      kShadow);
  view->layer()->SetRoundedCornerRadius(gfx::RoundedCornersF(
      layout_provider->GetCornerRadiusMetric(kRoundedCornersBorderRadius)));
  view->layer()->SetIsFastRoundedCorner(true);
  return shadow;
}

int BraveContentsViewUtil::GetRoundedCornersWebViewMargin(Browser* browser) {
  return BraveBrowserView::ShouldUseBraveWebViewRoundedCornersForContents(
             browser)
             ? BraveContentsViewUtil::kMarginThickness
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

  bool show_vertical_tab = tabs::utils::ShouldShowVerticalTabs(
      browser_window_interface->GetBrowserForMigrationOnly());
  auto* vertical_tab_strip_widget_delegate_view =
      browser_view->vertical_tab_strip_widget_delegate_view();

  // When hide completely is on, we think vertical tab is invisible
  // except it's expanded.
  if (show_vertical_tab && vertical_tab_strip_widget_delegate_view) {
    auto* vtsr_view = vertical_tab_strip_widget_delegate_view
                          ->vertical_tab_strip_region_view();
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

  if (browser_view->IsSidebarVisible()) {
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
