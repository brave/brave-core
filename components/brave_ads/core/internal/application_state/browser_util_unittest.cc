/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/application_state/browser_util.h"

#include "brave/components/brave_ads/core/internal/application_state/test/fake_browser_version.h"
#include "brave/components/brave_ads/core/internal/common/test/profile_pref_value_test_util.h"
#include "brave/components/brave_ads/core/internal/common/test/test_base.h"
#include "brave/components/brave_ads/core/internal/prefs/pref_util.h"
#include "brave/components/brave_ads/core/public/prefs/pref_names.h"
#include "testing/gtest/include/gtest/gtest.h"

// npm run test -- brave_unit_tests --filter=BraveAdsBrowserUtil*

namespace brave_ads {

class BraveAdsBrowserUtilTest : public test::TestBase {};

TEST_F(BraveAdsBrowserUtilTest, BrowserWasNotUpgradedWhenVersionHasNotChanged) {
  // Arrange
  fake_browser_version_.SetNumber(test::kDefaultBrowserVersionNumber);
  test::SetProfileStringPrefValue(prefs::kBrowserVersionNumber,
                                  test::kDefaultBrowserVersionNumber);

  // Act & Assert
  EXPECT_FALSE(WasBrowserUpgraded());
}

TEST_F(BraveAdsBrowserUtilTest, BrowserWasUpgradedWhenVersionChanges) {
  // Arrange
  test::SetProfileStringPrefValue(prefs::kBrowserVersionNumber, "1.0.0.0");
  fake_browser_version_.SetNumber("1.33.7.42");

  // Act
  EXPECT_TRUE(WasBrowserUpgraded());

  // Assert
  EXPECT_EQ("1.33.7.42", GetProfileStringPref(prefs::kBrowserVersionNumber));
}

}  // namespace brave_ads
