/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/omnibox/brave_rounded_omnibox_results_frame.h"

#include <memory>
#include <utility>

#include "brave/browser/ui/tabs/brave_tab_prefs.h"
#include "brave/browser/ui/views/tabs/vertical_tab_utils.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/views/location_bar/location_bar_view.h"
#include "ui/base/metadata/metadata_impl_macros.h"

BraveRoundedOmniboxResultsFrame::BraveRoundedOmniboxResultsFrame(
    views::View* contents,
    LocationBarView* location_bar)
    : RoundedOmniboxResultsFrame(contents, location_bar),
      browser_(location_bar->browser()) {
  UpdateShadowBorder();

  show_vertical_tabs_.Init(
      brave_tabs::kVerticalTabsEnabled,
      browser_->profile()->GetOriginalProfile()->GetPrefs(),
      base::BindRepeating(&BraveRoundedOmniboxResultsFrame::UpdateShadowBorder,
                          base::Unretained(this)));
  show_window_title_for_vertical_tabs_.Init(
      brave_tabs::kVerticalTabsShowTitleOnWindow,
      browser_->profile()->GetOriginalProfile()->GetPrefs(),
      base::BindRepeating(&BraveRoundedOmniboxResultsFrame::UpdateShadowBorder,
                          base::Unretained(this)));
}

BraveRoundedOmniboxResultsFrame::~BraveRoundedOmniboxResultsFrame() = default;

void BraveRoundedOmniboxResultsFrame::UpdateShadowBorder() {
  int corner_radius = views::LayoutProvider::Get()->GetCornerRadiusMetric(
      views::Emphasis::kHigh);

  auto border = std::make_unique<views::BubbleBorder>(
      views::BubbleBorder::Arrow::NONE,
      views::BubbleBorder::Shadow::STANDARD_SHADOW);
  border->SetCornerRadius(corner_radius);
  border->set_md_shadow_elevation(GetShadowElevation());
  if (tabs::utils::ShouldShowVerticalTabs(browser_) &&
      !tabs::utils::ShouldShowWindowTitleForVerticalTabs(browser_)) {
    // Remove top shadow inset so that omnibox popup stays inside browser
    // widget. Especially on Mac, Widgets can't be out of screen so we need to
    // adjust popup position.
    // https://github.com/brave/brave-browser/issues/26573
    border->set_insets(
        RoundedOmniboxResultsFrame::GetShadowInsets().set_top(0));
  }
  SetBorder(std::move(border));
}

BEGIN_METADATA(BraveRoundedOmniboxResultsFrame)
END_METADATA
