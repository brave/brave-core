/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "chrome/browser/ui/views/frame/vertical_tab_strip_region_view.h"

#include <string_view>

#include "base/feature_list.h"
#include "base/functional/bind.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/tabs/features.h"
#include "chrome/browser/ui/tabs/vertical_tab_strip_state_controller.h"
#include "chrome/browser/ui/views/frame/vertical_tab_strip_region_view.h"
#include "components/prefs/pref_service.h"
#include "ui/compositor/layer.h"
#include "ui/display/screen.h"
#include "ui/gfx/geometry/rect_f.h"

// Defined in //brave/browser/ui/views/tabs/vertical_tab_utils.cc.
std::string_view BraveVerticalTabsHideCompletelyPrefName();
bool BraveIsInVerticalTabHotCorner(const gfx::PointF& point_in_screen,
                                   BrowserView* browser_view,
                                   bool is_on_right);

namespace {

// Hides the whole region when it's collapsed in "hide completely" mode, and
// clips descendants to its bounds while that mode is on. Tab favicons and
// buttons don't shrink with the region, so without clipping they'd overflow
// the bounds (e.g. while the width animates) and paint over the web contents.
// The region already paints to a layer in upstream, so only the clipping needs
// to be enabled.
void MaybeUpdateVisibility(
    tabs::VerticalTabStripStateController* state_controller,
    VerticalTabStripRegionView* view) {
  const bool hide_completely =
      state_controller->ShouldHideCompletelyWhenCollapsed();
  view->layer()->SetMasksToBounds(hide_completely);
  view->SetVisible(!hide_completely || !state_controller->IsCollapsed());
}

}  // namespace

#include <chrome/browser/ui/views/frame/vertical_tab_strip_region_view.cc>

int VerticalTabStripRegionView::GetCollapsedWidth() const {
  return state_controller_->ShouldHideCompletelyWhenCollapsed()
             ? 0
             : kCollapsedWidth;
}

void VerticalTabStripRegionView::ObserveHideCompletelyPref() {
  if (!base::FeatureList::IsEnabled(tabs::kBraveVerticalTabHideCompletely)) {
    return;
  }

  // We assume SetPaintToLayer() was called for this feature - upstream
  // constructor is calling this.
  CHECK(layer());

  PrefService* prefs = browser_view()->GetProfile()->GetPrefs();
  hide_completely_pref_registrar_.Init(prefs);
  hide_completely_pref_registrar_.Add(
      BraveVerticalTabsHideCompletelyPrefName(),
      base::BindRepeating(
          &VerticalTabStripRegionView::OnHideCompletelyPrefChanged,
          base::Unretained(this)));
  MaybeUpdateVisibility(state_controller_, this);
}

void VerticalTabStripRegionView::OnHideCompletelyPrefChanged() {
  OnExpandOnHoverEnabledChanged(state_controller_->IsExpandOnHoverEnabled());
  MaybeUpdateVisibility(state_controller_, this);
  PreferredSizeChanged();
}

void VerticalTabStripRegionView::HandleMouseMoveEvent(
    const gfx::PointF& point_in_screen) {
  if (!state_controller_->ShouldDisplayVerticalTabs() ||
      !state_controller_->ShouldHideCompletelyWhenCollapsed() ||
      !state_controller_->IsCollapsed()) {
    return;
  }

  UpdateExpandOnHoverState(
      IsMouseHovered() ||
      BraveIsInVerticalTabHotCorner(point_in_screen, browser_view(),
                    state_controller_->IsVerticalTabOnRight()));
}

bool VerticalTabStripRegionView::IsMouseInHotCorner() const {
  if (!state_controller_->ShouldHideCompletelyWhenCollapsed()) {
    return false;
  }

  return BraveIsInVerticalTabHotCorner(
      gfx::PointF(display::Screen::Get()->GetCursorScreenPoint()),
      browser_view(),
      state_controller_->IsVerticalTabOnRight());
}

bool VerticalTabStripRegionView::IsVerticalTabStripLeading() const {
  // Region view bounds are in layout coordinates, where the leading edge is
  // the physical left in LTR and the physical right in RTL. Flip the pref in
  // RTL so the strip always ends up on the physical side the user chose.
  return state_controller_->IsVerticalTabOnRight() == base::i18n::IsRTL();
}

void VerticalTabStripRegionView::ObserveVerticalTabOnRightPref() {
  vertical_tab_on_right_pref_registrar_.Init(
      browser_view()->GetProfile()->GetPrefs());
  vertical_tab_on_right_pref_registrar_.Add(
      brave_tabs::kVerticalTabsOnRight,
      base::BindRepeating(
          &VerticalTabStripRegionView::OnVerticalTabOnRightPrefChanged,
          base::Unretained(this)));
}

void VerticalTabStripRegionView::OnVerticalTabOnRightPrefChanged() {
  InvalidateLayout();
  PreferredSizeChanged();
}
