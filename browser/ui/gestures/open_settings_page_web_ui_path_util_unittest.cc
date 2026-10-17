// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/ui/gestures/open_settings_page_web_ui_path_util.h"

#include "testing/gtest/include/gtest/gtest.h"

namespace gestures {

TEST(OpenSettingsPageWebUIPathUtilTest, PageToWebUIPathReturnsEmptyForBasic) {
  EXPECT_EQ("", PageToWebUIPath(OpenSettingsPage::kBasic));
}

TEST(OpenSettingsPageWebUIPathUtilTest, PageToWebUIPathReturnsTheRealPath) {
  EXPECT_EQ("privacy", PageToWebUIPath(OpenSettingsPage::kPrivacy));
  EXPECT_EQ("system/gestures", PageToWebUIPath(OpenSettingsPage::kGestures));
}

TEST(OpenSettingsPageWebUIPathUtilTest,
     PageToWebUIPathReturnsEmptyForAnOutOfRangeValue) {
  EXPECT_EQ("", PageToWebUIPath(static_cast<OpenSettingsPage>(999999)));
}

}  // namespace gestures
