// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/ui/tabs/public/vertical_tab_controller.h"

#include <memory>

#include "base/test/scoped_feature_list.h"
#include "brave/browser/ui/tabs/brave_tab_prefs.h"
#include "chrome/browser/ui/browser_window/public/browser_window_interface.h"
#include "chrome/browser/ui/tabs/features.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/prefs/testing_pref_service.h"
#include "testing/gtest/include/gtest/gtest.h"

class VerticalTabControllerUnitTest : public testing::Test {
 public:
  void SetUp() override {
    brave_tabs::RegisterBraveProfilePrefs(pref_service_.registry());
  }

 protected:
  std::unique_ptr<VerticalTabController> MakeController(
      BrowserWindowInterface::Type type = BrowserWindowInterface::TYPE_NORMAL) {
    return std::make_unique<VerticalTabController>(type, &pref_service_);
  }

  TestingPrefServiceSimple pref_service_;
};

TEST_F(VerticalTabControllerUnitTest, SupportsBraveVerticalTabsNormalWindow) {
  auto controller = MakeController(BrowserWindowInterface::TYPE_NORMAL);
  EXPECT_TRUE(controller->SupportsBraveVerticalTabs());
}

TEST_F(VerticalTabControllerUnitTest, SupportsBraveVerticalTabsPopupWindow) {
  auto controller = MakeController(BrowserWindowInterface::TYPE_POPUP);
  EXPECT_FALSE(controller->SupportsBraveVerticalTabs());
}

TEST_F(VerticalTabControllerUnitTest, ShouldShowBraveVerticalTabsDefaultOff) {
  auto controller = MakeController();
  EXPECT_FALSE(controller->ShouldShowBraveVerticalTabs());
}

TEST_F(VerticalTabControllerUnitTest, ShouldShowBraveVerticalTabsWhenEnabled) {
  pref_service_.SetBoolean(brave_tabs::kVerticalTabsEnabled, true);
  auto controller = MakeController();
  EXPECT_TRUE(controller->ShouldShowBraveVerticalTabs());
}

TEST_F(VerticalTabControllerUnitTest,
       ShouldShowBraveVerticalTabsFalseForPopupEvenIfPrefEnabled) {
  pref_service_.SetBoolean(brave_tabs::kVerticalTabsEnabled, true);
  auto controller = MakeController(BrowserWindowInterface::TYPE_POPUP);
  EXPECT_FALSE(controller->ShouldShowBraveVerticalTabs());
}

TEST_F(VerticalTabControllerUnitTest, ShouldShowWindowTitleWhenEnabled) {
  pref_service_.SetBoolean(brave_tabs::kVerticalTabsEnabled, true);
  pref_service_.SetBoolean(brave_tabs::kVerticalTabsShowTitleOnWindow, true);
  auto controller = MakeController();
  EXPECT_TRUE(controller->ShouldShowWindowTitleForVerticalTabs());
}

TEST_F(VerticalTabControllerUnitTest,
       ShouldShowWindowTitleFalseWhenVerticalTabsDisabled) {
  pref_service_.SetBoolean(brave_tabs::kVerticalTabsEnabled, false);
  pref_service_.SetBoolean(brave_tabs::kVerticalTabsShowTitleOnWindow, true);
  auto controller = MakeController();
  EXPECT_FALSE(controller->ShouldShowWindowTitleForVerticalTabs());
}

TEST_F(VerticalTabControllerUnitTest, IsFloatingVerticalTabsEnabledDefault) {
  pref_service_.SetBoolean(brave_tabs::kVerticalTabsEnabled, true);
  // kVerticalTabsFloatingEnabled defaults to true
  auto controller = MakeController();
  EXPECT_TRUE(controller->IsFloatingVerticalTabsEnabled());
}

TEST_F(VerticalTabControllerUnitTest,
       IsFloatingVerticalTabsDisabledWhenVerticalTabsOff) {
  auto controller = MakeController();
  EXPECT_FALSE(controller->IsFloatingVerticalTabsEnabled());
}

TEST_F(VerticalTabControllerUnitTest, IsVerticalTabOnRightDefault) {
  auto controller = MakeController();
  EXPECT_FALSE(controller->IsVerticalTabOnRight());
}

TEST_F(VerticalTabControllerUnitTest, IsVerticalTabOnRightWhenSet) {
  pref_service_.SetBoolean(brave_tabs::kVerticalTabsOnRight, true);
  auto controller = MakeController();
  EXPECT_TRUE(controller->IsVerticalTabOnRight());
}
