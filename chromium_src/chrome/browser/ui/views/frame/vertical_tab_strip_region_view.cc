/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "chrome/browser/ui/views/frame/vertical_tab_strip_region_view.h"

#include "base/feature_list.h"
#include "base/functional/bind.h"
#include "brave/browser/ui/tabs/brave_tab_prefs.h"
#include "brave/browser/ui/views/frame/brave_browser_view.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/tabs/features.h"
#include "components/prefs/pref_service.h"
#include "ui/display/screen.h"
#include "ui/gfx/geometry/rect_f.h"

#include <chrome/browser/ui/views/frame/vertical_tab_strip_region_view.cc>

int VerticalTabStripRegionView::GetCollapsedWidth() const {
  return state_controller_->ShouldHideCompletelyWhenCollapsed()
             ? 0
             : kCollapsedWidth;
}

void VerticalTabStripRegionView::ObserveHideCompletelyPref() {
  PrefService* prefs = browser_view()->GetProfile()->GetPrefs();
  if (!base::FeatureList::IsEnabled(tabs::kBraveVerticalTabHideCompletely) ||
      !prefs->FindPreference(
          brave_tabs::kVerticalTabsHideCompletelyWhenCollapsed)) {
    return;
  }
  hide_completely_pref_registrar_.Init(prefs);
  hide_completely_pref_registrar_.Add(
      brave_tabs::kVerticalTabsHideCompletelyWhenCollapsed,
      base::BindRepeating(
          &VerticalTabStripRegionView::OnHideCompletelyPrefChanged,
          base::Unretained(this)));
}

void VerticalTabStripRegionView::OnHideCompletelyPrefChanged() {
  OnExpandOnHoverEnabledChanged(state_controller_->IsExpandOnHoverEnabled());
  PreferredSizeChanged();
}

void VerticalTabStripRegionView::HandleMouseMoveEvent(
    const gfx::PointF& point_in_screen) {
  if (!state_controller_->ShouldDisplayVerticalTabs() ||
      !state_controller_->ShouldHideCompletelyWhenCollapsed() ||
      !state_controller_->IsCollapsed()) {
    return;
  }

  gfx::RectF hot_corner(BraveBrowserView::From(browser_view())
                            ->GetBoundingBoxInScreenForMouseOverHandling());
  constexpr int kHotCornerWidth = 7;
  hot_corner.set_width(kHotCornerWidth);
  UpdateExpandOnHoverState(
      hot_corner.Contains(point_in_screen) ||
      (is_expanded_on_hover_ &&
       gfx::RectF(GetBoundsInScreen()).Contains(point_in_screen)));
}

bool VerticalTabStripRegionView::IsMouseInHotCorner() const {
  if (!state_controller_->ShouldHideCompletelyWhenCollapsed()) {
    return false;
  }
  gfx::RectF hot_corner(BraveBrowserView::From(browser_view())
                            ->GetBoundingBoxInScreenForMouseOverHandling());
  constexpr int kHotCornerWidth = 7;
  hot_corner.set_width(kHotCornerWidth);
  return hot_corner.Contains(
      gfx::PointF(display::Screen::Get()->GetCursorScreenPoint()));
}
