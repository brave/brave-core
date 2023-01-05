// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#include "brave/browser/ui/views/tabs/brave_tab.h"

#include <algorithm>

#include "brave/browser/ui/tabs/brave_tab_prefs.h"
#include "brave/browser/ui/views/tabs/features.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/views/tabs/alert_indicator_button.h"
#include "chrome/browser/ui/views/tabs/tab_close_button.h"
#include "chrome/browser/ui/views/tabs/tab_slot_controller.h"

BraveTab::~BraveTab() = default;

std::u16string BraveTab::GetTooltipText(const gfx::Point& p) const {
  auto* browser = controller_->GetBrowser();
  if (browser &&
      brave_tabs::AreTooltipsEnabled(browser->profile()->GetPrefs())) {
    return Tab::GetTooltipText(data_.title,
                               GetAlertStateToShow(data_.alert_state));
  }
  return Tab::GetTooltipText(p);
}

int BraveTab::GetWidthOfLargestSelectableRegion() const {
  // Assume the entire region except the area that alert indicator/close buttons
  // occupied is available for click-to-select.
  // If neither are visible, the entire tab region is available.
  int selectable_width = width();
  if (alert_indicator_button_->GetVisible()) {
    selectable_width -= alert_indicator_button_->width();
  }

  if (close_button_->GetVisible())
    selectable_width -= close_button_->width();

  return std::max(0, selectable_width);
}

void BraveTab::ActiveStateChanged() {
  Tab::ActiveStateChanged();
  // This should be called whenever acitve state changes
  // see comment on UpdateEnabledForMuteToggle();
  // https://github.com/brave/brave-browser/issues/23476/
  alert_indicator_button_->UpdateEnabledForMuteToggle();
}

absl::optional<SkColor> BraveTab::GetGroupColor() const {
  if (!base::FeatureList::IsEnabled(tabs::features::kBraveVerticalTabs))
    return Tab::GetGroupColor();

  // Hide tab border with group color as it doesn't go well with vertical tabs.
  if (tabs::features::ShouldShowVerticalTabs(controller()->GetBrowser()))
    return {};

  return Tab::GetGroupColor();
}

void BraveTab::UpdateIconVisibility() {
  Tab::UpdateIconVisibility();
  if (IsAtMinWidthForVerticalTabStrip()) {
    if (data().pinned) {
      center_icon_ = true;
      showing_icon_ = !showing_alert_indicator_;
      showing_close_button_ = false;
    } else {
      const bool is_active = IsActive();
      center_icon_ = true;
      showing_icon_ = !is_active && !showing_alert_indicator_;
      showing_close_button_ = is_active;
    }
  }
}

void BraveTab::Layout() {
  Tab::Layout();
  if (IsAtMinWidthForVerticalTabStrip()) {
    if (showing_close_button_) {
      close_button_->SetX(GetLocalBounds().CenterPoint().x() -
                          (close_button_->width() / 2));
      close_button_->SetButtonPadding({});
    }
  }
}

bool BraveTab::ShouldRenderAsNormalTab() const {
  if (IsAtMinWidthForVerticalTabStrip()) {
    // Returns false to hide title
    return false;
  }

  return Tab::ShouldRenderAsNormalTab();
}

bool BraveTab::IsAtMinWidthForVerticalTabStrip() const {
  if (!base::FeatureList::IsEnabled(tabs::features::kBraveVerticalTabs))
    return false;

  return tabs::features::ShouldShowVerticalTabs(controller()->GetBrowser()) &&
         width() <= tabs::kVerticalTabMinWidth;
}
