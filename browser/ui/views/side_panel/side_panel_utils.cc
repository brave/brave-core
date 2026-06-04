/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/side_panel/side_panel_utils.h"

#include "brave/common/pref_names.h"
#include "chrome/browser/ui/views/chrome_layout_provider.h"
#include "components/prefs/pref_service.h"
#include "ui/views/layout/layout_provider.h"

namespace brave {

bool ShouldShowSidePanelHeader(SidePanelEntryId id) {
  return id == SidePanelEntryId::kReadingList ||
         id == SidePanelEntryId::kBookmarks;
}

gfx::RoundedCornersF GetPanelContentsRoundedCorners(PrefService* prefs,
                                                    bool has_header) {
  // When Brave's rounded corners are off, the panel has no rounded border, so
  // the content shouldn't be rounded either.
  if (!prefs->GetBoolean(kWebViewRoundedCorners)) {
    return gfx::RoundedCornersF();
  }

  gfx::RoundedCornersF radii(views::LayoutProvider::Get()->GetDistanceMetric(
      ChromeDistanceMetric::DISTANCE_SIDE_PANEL_CONTENT_RADIUS));

  // When a Brave header is attached it paints the rounded top, so flatten the
  // content's top corners to avoid double-rounding.
  if (has_header) {
    radii.set_upper_left(0);
    radii.set_upper_right(0);
  }

  return radii;
}

}  // namespace brave
