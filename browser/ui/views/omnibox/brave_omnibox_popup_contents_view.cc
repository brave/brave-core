/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/omnibox/brave_omnibox_popup_contents_view.h"

#include "brave/browser/ui/views/tabs/features.h"
#include "chrome/browser/ui/views/location_bar/location_bar_view.h"
#include "chrome/browser/ui/views/omnibox/rounded_omnibox_results_frame.h"
#include "ui/base/metadata/metadata_impl_macros.h"

BraveOmniboxPopupContentsView::~BraveOmniboxPopupContentsView() = default;

gfx::Rect BraveOmniboxPopupContentsView::GetTargetBounds() const {
  auto bounds = OmniboxPopupContentsView::GetTargetBounds();
  if (!base::FeatureList::IsEnabled(tabs::features::kBraveVerticalTabs))
    return bounds;

  if (auto* browser = location_bar_view_->browser();
      tabs::features::ShouldShowVerticalTabs(browser) &&
      !tabs::features::ShouldShowWindowTitleForVerticalTabs(browser)) {
    // Remove top shadow inset so that omnibox popup stays inside browser
    // widget. Especially on Mac, Widgets can't be out of screen so we need to
    // adjust popup position.
    // https://github.com/brave/brave-browser/issues/26573
    bounds.Inset(gfx::Insets().set_top(
        RoundedOmniboxResultsFrame::GetShadowInsets().top()));
  }

  return bounds;
}

BEGIN_METADATA(BraveOmniboxPopupContentsView, OmniboxPopupContentsView)
END_METADATA
