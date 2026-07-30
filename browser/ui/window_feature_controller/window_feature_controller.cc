/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/tabs/public/vertical_tab_controller.h"
#include "chrome/browser/ui/layout_constants.h"

bool BraveDisablesImmersiveFullscreenMode(
    const base::WeakPtr<VerticalTabController>& vertical_tab_controller) {
  return (vertical_tab_controller &&
          vertical_tab_controller->ShouldShowBraveVerticalTabs()) ||
         tabs::UseCompactHorizontalTabs();
}

bool BraveShouldShowTitlebar(
    const base::WeakPtr<VerticalTabController>& vertical_tab_controller) {
  return vertical_tab_controller &&
         vertical_tab_controller->ShouldShowBraveVerticalTabs() &&
         vertical_tab_controller->ShouldShowWindowTitleForVerticalTabs();
}
