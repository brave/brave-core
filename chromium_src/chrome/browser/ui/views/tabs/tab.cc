/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "chrome/browser/ui/views/tabs/tab.h"

#include "brave/browser/ui/tabs/public/vertical_tab_controller.h"
#include "brave/browser/ui/views/tabs/brave_tab_strip_layout_helper.h"
#include "brave/components/constants/pref_names.h"
#include "chrome/browser/ui/layout_constants.h"

#include <chrome/browser/ui/views/tabs/tab.cc>

ControllableCloseButtonState::ControllableCloseButtonState(
    TabSlotController& controller,
    Tab& tab)
    : controller(controller), tab(tab) {}

ControllableCloseButtonState::~ControllableCloseButtonState() = default;

bool ControllableCloseButtonState::operator=(bool show) {
  showing_close_button = show;
  return showing_close_button;
}

ControllableCloseButtonState::operator bool() const {
  return !controller->ShouldAlwaysHideCloseButton() &&
         (tab->IsActive() || tab->mouse_hovered()) && showing_close_button;
}

bool Tab::IsTabMuteIndicatorNotClickable() {
  auto* browser = controller()->GetBrowserWindowInterface();
  // We have clickable mute indicators enabled by default. Thus, if our pref is
  // disabled we can force the indicator off.
  // Note: We have a test which checks the feature is enabled by default. If
  // that changes this may need to as well.
  // Note: |browser| is |nullptr| in some unit_tests.
  return browser && browser->GetProfile()->GetPrefs()->GetBoolean(
                        kTabMuteIndicatorNotClickable);
}

void Tab::ResetTabStyle(std::unique_ptr<TabStyleViews> new_style) {
  tab_style_views_ = std::move(new_style);

  views::HighlightPathGenerator::Install(
      this,
      std::make_unique<TabStyleHighlightPathGenerator>(tab_style_views()));
}

int Tab::GetTabHeight() const {
  auto* vertical_tab_controller = VerticalTabController::FromBrowser(
      controller()->GetBrowserWindowInterface());
  if (vertical_tab_controller &&
      vertical_tab_controller->ShouldShowBraveVerticalTabs()) {
    return tabs::kVerticalTabHeight;
  }
  return GetLayoutConstant(LayoutConstant::kTabHeight);
}
