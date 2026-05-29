/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_VIEWS_SIDE_PANEL_SIDE_PANEL_UTILS_H_
#define BRAVE_BROWSER_UI_VIEWS_SIDE_PANEL_SIDE_PANEL_UTILS_H_

#include "chrome/browser/ui/side_panel/side_panel_entry_id.h"
#include "ui/gfx/geometry/rounded_corners_f.h"

class PrefService;

namespace brave {

// Returns the rounded corners for the web content hosted inside the side panel.
// Returns empty corners when Brave's rounded corners are disabled. When
// `has_header` is true the Brave header owns the top rounding, so the
// content's top corners are flattened to avoid double-rounding.
gfx::RoundedCornersF GetPanelContentsRoundedCorners(PrefService* prefs,
                                                    bool has_header);

// Returns true when the entry with `id` should show Brave's custom side panel
// header view.
bool ShouldShowSidePanelHeader(SidePanelEntryId id);

}  // namespace brave

#endif  // BRAVE_BROWSER_UI_VIEWS_SIDE_PANEL_SIDE_PANEL_UTILS_H_
