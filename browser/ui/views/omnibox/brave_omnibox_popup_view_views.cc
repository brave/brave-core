/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/omnibox/brave_omnibox_popup_view_views.h"

#include "brave/browser/ui/views/tabs/vertical_tab_controller.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_window/public/browser_window_features.h"
#include "chrome/browser/ui/views/location_bar/location_bar_view.h"
#include "chrome/browser/ui/views/omnibox/rounded_omnibox_results_frame.h"
#include "ui/base/metadata/metadata_impl_macros.h"

BraveOmniboxPopupViewViews::~BraveOmniboxPopupViewViews() = default;

gfx::Rect BraveOmniboxPopupViewViews::GetTargetBounds() const {
  auto bounds = OmniboxPopupViewViews::GetTargetBounds();
  if (auto* browser = location_bar_view_->browser();
      browser->GetFeatures()
          .vertical_tab_controller()
          ->ShouldShowBraveVerticalTabs() &&
      !browser->GetFeatures()
           .vertical_tab_controller()
           ->ShouldShowWindowTitleForVerticalTabs()) {
    // Remove top shadow inset so that omnibox popup stays inside browser
    // widget. Especially on Mac, Widgets can't be out of screen so we need to
    // adjust popup position.
    // https://github.com/brave/brave-browser/issues/26573
    bounds.Inset(gfx::Insets().set_top(
        RoundedOmniboxResultsFrame::GetShadowInsets().top()));
  }

  return bounds;
}

int BraveOmniboxPopupViewViews::GetLocationBarViewWidth() const {
  return location_bar_view()->width();
}

BEGIN_METADATA(BraveOmniboxPopupViewViews)
END_METADATA
