// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/ui/brave_browser_command_controller.h"

#include "brave/app/brave_command_ids.h"
#include "brave/browser/ui/browser_commands.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_window.h"
#include "chrome/app/chrome_command_ids.h"

BraveBrowserCommandController::BraveBrowserCommandController(Browser* browser)
    : BrowserCommandController(browser) {}

BraveBrowserCommandController::~BraveBrowserCommandController() = default;

bool BraveBrowserCommandController::ExecuteCommand(int id, int event_flags) {
  if (id == IDC_FOCUS_LOCATION && browser()->window()->IsFullscreen()) {
    brave::FocusLocationBarInFullscreen(browser());
    return true;
  }

  if (ExecuteBraveCommand(id, event_flags))
    return true;

  return BrowserCommandController::ExecuteCommand(id, event_flags);
}

void BraveBrowserCommandController::InitCommandState() {
  BrowserCommandController::InitCommandState();
  command_updater()->UpdateCommandEnabled(IDC_FOCUS_LOCATION_FULLSCREEN, true);
}

bool BraveBrowserCommandController::ExecuteBraveCommand(int id, int event_flags) {
  switch (id) {
    case IDC_FOCUS_LOCATION_FULLSCREEN:
      brave::FocusLocationBarInFullscreen(browser());
      return true;
    default:
      return false;
  }
}