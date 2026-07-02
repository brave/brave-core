/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/side_panel/side_panel_utils.h"

#include "brave/browser/ui/views/frame/brave_browser_view.h"
#include "brave/common/pref_names.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/views/side_panel/side_panel.h"
#include "chrome/common/pref_names.h"
#include "components/prefs/pref_service.h"
#include "ui/views/layout/layout_provider.h"
#include "ui/views/view.h"

using views::ShapeContextTokensOverride::kRoundedCornersBorderRadius;
using views::ShapeContextTokensOverride::
    kRoundedCornersBorderRadiusAtWindowCorner;

namespace brave {

bool ShouldShowSidePanelHeader(SidePanelEntryId id) {
  return id == SidePanelEntryId::kReadingList ||
         id == SidePanelEntryId::kBookmarks;
}

gfx::RoundedCornersF GetPanelContentsRoundedCorners(
    BrowserView* browser_view,
    bool flatten_top_for_header) {
  auto* brave_browser_view = BraveBrowserView::From(browser_view);
  CHECK(brave_browser_view);
  auto* prefs = brave_browser_view->GetProfile()->GetPrefs();
  const bool has_header =
      browser_view->side_panel()->GetHeaderView<views::View>() != nullptr;

  // When Brave's rounded corners are off, the panel has no rounded border, so
  // the content shouldn't be rounded either.
  if (!prefs->GetBoolean(kWebViewRoundedCorners)) {
    return gfx::RoundedCornersF();
  }

  auto* layout_provider = views::LayoutProvider::Get();
  gfx::RoundedCornersF rounded_corners(
      layout_provider->GetCornerRadiusMetric(kRoundedCornersBorderRadius));

  // When a Brave header is attached it paints the rounded top, so flatten the
  // content's top corners to avoid double-rounding.
  if (has_header && flatten_top_for_header) {
    rounded_corners.set_upper_left(0);
    rounded_corners.set_upper_right(0);
  }

  // When the sidebar is visible it sits between the panel and the window edge,
  // so the panel's bottom corner is not a window corner — use regular radius.
  if (brave_browser_view->IsSidebarVisible()) {
    return rounded_corners;
  }

  // When the sidebar is hidden the panel touches the window edge directly. The
  // inner bottom corner (lower-right for a right-aligned panel, lower-left for
  // a left-aligned panel) is the window corner and must use the larger
  // window-corner radius.
  const int rounded_corners_border_radius_at_window_corner =
      layout_provider->GetCornerRadiusMetric(
          kRoundedCornersBorderRadiusAtWindowCorner);

  const bool panel_on_right =
      prefs->GetBoolean(prefs::kSidePanelHorizontalAlignment);
  if (panel_on_right) {
    rounded_corners.set_lower_right(
        rounded_corners_border_radius_at_window_corner);
  } else {
    rounded_corners.set_lower_left(
        rounded_corners_border_radius_at_window_corner);
  }

  return rounded_corners;
}

}  // namespace brave
