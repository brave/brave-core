/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/frame/brave_contents_view_util.h"

#include "base/check.h"
#include "brave/browser/ui/views/frame/brave_browser_view.h"
#include "brave/browser/ui/views/tabs/vertical_tab_utils.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "chrome/common/pref_names.h"
#include "components/prefs/pref_service.h"
#include "components/tabs/public/split_tab_data.h"
#include "components/tabs/public/split_tab_id.h"
#include "ui/compositor/layer.h"
#include "ui/gfx/geometry/rounded_corners_f.h"
#include "ui/views/view.h"

std::unique_ptr<ViewShadow> BraveContentsViewUtil::CreateShadow(
    views::View* view) {
  static const ViewShadow::ShadowParameters kShadow{
      .offset_x = 0,
      .offset_y = 0,
      .blur_radius = BraveContentsViewUtil::GetMargin(),
      .shadow_color = SkColorSetA(SK_ColorBLACK, 0.1 * 255)};

  DCHECK(view);
  auto shadow = std::make_unique<ViewShadow>(view, GetBorderRadius(), kShadow);
  view->layer()->SetRoundedCornerRadius(
      gfx::RoundedCornersF(GetBorderRadius()));
  view->layer()->SetIsFastRoundedCorner(true);
  return shadow;
}

int BraveContentsViewUtil::GetRoundedCornersWebViewMargin(Browser* browser) {
  return BraveBrowserView::ShouldUseBraveWebViewRoundedCornersForContents(
             browser)
             ? BraveContentsViewUtil::GetMargin()
             : 0;
}

// static
int BraveContentsViewUtil::GetMargin() {
  return 4;
}

// static
gfx::RoundedCornersF BraveContentsViewUtil::GetRoundedCornersForContentsView(
    Browser* browser,
    tabs::TabInterface* split_tab) {
  auto rounded_corners = gfx::RoundedCornersF(GetBorderRadiusAroundWindow());
  rounded_corners.set_upper_left(GetBorderRadius());
  rounded_corners.set_upper_right(GetBorderRadius());

  const bool show_vertical_tab = tabs::utils::ShouldShowVerticalTabs(browser);
  auto* browser_view =
      BraveBrowserView::From(BrowserView::GetBrowserViewForBrowser(browser));

  // Can null during the startup.
  if (!browser_view) {
    return rounded_corners;
  }

  const bool is_sidebar_visible = browser_view->IsSidebarVisible();

  bool has_left_side_ui = false;
  bool has_right_side_ui = false;

  if (show_vertical_tab) {
    if (tabs::utils::IsVerticalTabOnRight(browser)) {
      has_right_side_ui = true;
    } else {
      has_left_side_ui = true;
    }
  }

  if (is_sidebar_visible) {
    if (browser->profile()->GetPrefs()->GetBoolean(
            prefs::kSidePanelHorizontalAlignment)) {
      has_right_side_ui = true;
    } else {
      has_left_side_ui = true;
    }
  }

  if (split_tab) {
    CHECK(split_tab->IsSplit());
    auto split_tab_id = split_tab->GetSplit();
    auto* tab_strip_model = browser->tab_strip_model();
    auto* split_data = tab_strip_model->GetSplitData(*split_tab_id);

    // Handle each split tab's lower edge.
    if (split_data->ListTabs()[0] == split_tab) {
      has_right_side_ui = true;
    } else {
      has_left_side_ui = true;
    }
  }

  if (has_right_side_ui) {
    rounded_corners.set_lower_right(GetBorderRadius());
  }

  if (has_left_side_ui) {
    rounded_corners.set_lower_left(GetBorderRadius());
  }

  return rounded_corners;
}

#if !BUILDFLAG(IS_MAC)

// static
int BraveContentsViewUtil::GetBorderRadius() {
  return 4;
}

// static
int BraveContentsViewUtil::GetBorderRadiusAroundWindow() {
  return 4;
}
#endif
