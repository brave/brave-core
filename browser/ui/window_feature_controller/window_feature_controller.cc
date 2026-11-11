/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/tabs/public/vertical_tab_controller.h"
#include "chrome/browser/ui/layout_constants.h"

bool BraveDisablesImmersiveFullscreenMode(const ui::UnownedUserDataHost& host) {
  const auto* vertical_tab_controller = VerticalTabController::Get(host);
  return (vertical_tab_controller &&
          vertical_tab_controller->ShouldShowBraveVerticalTabs()) ||
         tabs::UseCompactHorizontalTabs();
}

bool BraveShouldShowTitlebar(const ui::UnownedUserDataHost& host) {
  const auto* vertical_tab_controller = VerticalTabController::Get(host);
  return vertical_tab_controller &&
         vertical_tab_controller->ShouldShowBraveVerticalTabs() &&
         vertical_tab_controller->ShouldShowWindowTitleForVerticalTabs();
}
