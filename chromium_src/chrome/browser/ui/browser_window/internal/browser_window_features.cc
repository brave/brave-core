/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "chrome/browser/ui/browser_window/public/browser_window_features.h"

#include "brave/browser/ui/brave_browser_actions.h"
#include "brave/browser/ui/brave_browser_command_controller.h"
#include "brave/browser/ui/brave_browser_content_setting_bubble_model_delegate.h"
#include "brave/browser/ui/brave_browser_web_contents_delegate.h"
#include "brave/browser/ui/focus_mode/focus_mode_controller.h"
#include "brave/browser/ui/focus_mode/focus_mode_utils.h"
#include "brave/browser/ui/tabs/public/vertical_tab_controller.h"
#include "brave/browser/ui/toolbar/brave_location_bar_model_delegate.h"
#include "brave/browser/ui/views/side_panel/brave_side_panel_coordinator.h"
#include "chrome/browser/profiles/profile.h"

namespace {

std::unique_ptr<FocusModeController> CreateBraveFocusModeController(
    ui::UserDataFactoryWithOwner<BrowserWindowInterface>& factory,
    BrowserWindowInterface* browser) {
  if (!BrowserSupportsFocusMode(browser)) {
    return nullptr;
  }
  return factory.CreateInstance<FocusModeController>(
      *browser, *browser, browser->GetProfile()->GetPrefs());
}

std::unique_ptr<VerticalTabController> CreateBraveVerticalTabController(
    ui::UserDataFactoryWithOwner<BrowserWindowInterface>& factory,
    BrowserWindowInterface* browser,
    FocusModeController* focus_mode_controller) {
  return factory.CreateInstance<VerticalTabController>(
      *browser, browser->GetUnownedUserDataHost(), browser->GetType(),
      browser->GetProfile()->GetPrefs(), focus_mode_controller);
}

}  // namespace

#include <chrome/browser/ui/browser_window/internal/browser_window_features.cc>
