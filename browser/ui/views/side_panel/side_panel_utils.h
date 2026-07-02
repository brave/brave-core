/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_VIEWS_SIDE_PANEL_SIDE_PANEL_UTILS_H_
#define BRAVE_BROWSER_UI_VIEWS_SIDE_PANEL_SIDE_PANEL_UTILS_H_

#include "chrome/browser/ui/side_panel/side_panel_entry_id.h"
#include "ui/gfx/geometry/rounded_corners_f.h"

class BrowserView;

namespace brave {

// Returns the rounded corners for the web content hosted inside the side panel.
// Returns empty corners when Brave's rounded corners are disabled. When the
// side panel has a Brave header attached it owns the top rounding, so by
// default the content's top corners are flattened to avoid double-rounding
// (pass `flatten_top_for_header = false` to get the un-flattened radius
// instead -- e.g. for a shape meant to wrap the header and content together,
// such as the side panel's own outline border). The bottom inner corner uses
// the window-corner radius when the sidebar is hidden (the panel is flush
// with the window edge) and the regular radius otherwise. The header state and
// prefs are read from `browser_view` (and its side panel).
gfx::RoundedCornersF GetPanelContentsRoundedCorners(
    BrowserView* browser_view,
    bool flatten_top_for_header = true);

// Returns true when the entry with `id` should show Brave's custom side panel
// header view.
bool ShouldShowSidePanelHeader(SidePanelEntryId id);

}  // namespace brave

#endif  // BRAVE_BROWSER_UI_VIEWS_SIDE_PANEL_SIDE_PANEL_UTILS_H_
