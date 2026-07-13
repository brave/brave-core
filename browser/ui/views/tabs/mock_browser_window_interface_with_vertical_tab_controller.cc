/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/tabs/mock_browser_window_interface_with_vertical_tab_controller.h"

#include <memory>

#include "brave/browser/ui/tabs/public/vertical_tab_controller.h"
#include "chrome/browser/ui/browser_window/public/browser_window_interface.h"
#include "testing/gmock/include/gmock/gmock.h"

MockBrowserWindowInterfaceWithVerticalTabController::
    MockBrowserWindowInterfaceWithVerticalTabController(PrefService* prefs) {
  features_.SetVerticalTabControllerForTesting(
      std::make_unique<VerticalTabController>(
          BrowserWindowInterface::TYPE_NORMAL, prefs, &focus_mode_controller_));
  ON_CALL(*this, GetFeatures()).WillByDefault(testing::ReturnRef(features_));
}

MockBrowserWindowInterfaceWithVerticalTabController::
    ~MockBrowserWindowInterfaceWithVerticalTabController() = default;
