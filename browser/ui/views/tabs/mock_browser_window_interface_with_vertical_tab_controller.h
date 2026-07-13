/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_VIEWS_TABS_MOCK_BROWSER_WINDOW_INTERFACE_WITH_VERTICAL_TAB_CONTROLLER_H_
#define BRAVE_BROWSER_UI_VIEWS_TABS_MOCK_BROWSER_WINDOW_INTERFACE_WITH_VERTICAL_TAB_CONTROLLER_H_

#include "brave/browser/ui/focus_mode/focus_mode_controller.h"
#include "chrome/browser/ui/browser_window/public/browser_window_features.h"
#include "chrome/browser/ui/browser_window/test/mock_browser_window_interface.h"

class PrefService;

class MockBrowserWindowInterfaceWithVerticalTabController
    : public MockBrowserWindowInterface {
 public:
  explicit MockBrowserWindowInterfaceWithVerticalTabController(
      PrefService* prefs);
  ~MockBrowserWindowInterfaceWithVerticalTabController() override;

 private:
  FocusModeController focus_mode_controller_;
  BrowserWindowFeatures features_;
};

#endif  // BRAVE_BROWSER_UI_VIEWS_TABS_MOCK_BROWSER_WINDOW_INTERFACE_WITH_VERTICAL_TAB_CONTROLLER_H_
