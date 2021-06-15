/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/frequency_capping/permission_rules/browser_is_active_frequency_cap.h"

#include <vector>

#include "base/test/scoped_feature_list.h"
#include "bat/ads/internal/browser_manager/browser_manager.h"
#include "bat/ads/internal/frequency_capping/frequency_capping_features.h"
#include "bat/ads/internal/unittest_base.h"
#include "bat/ads/internal/unittest_util.h"

// npm run test -- brave_unit_tests --filter=BatAds*

namespace ads {

class BatAdsBrowserIsActiveFrequencyCapTest : public UnitTestBase {
 protected:
  BatAdsBrowserIsActiveFrequencyCapTest() = default;

  ~BatAdsBrowserIsActiveFrequencyCapTest() override = default;
};

TEST_F(BatAdsBrowserIsActiveFrequencyCapTest, AllowAd) {
  // Arrange
  MockPlatformHelper(platform_helper_mock_, PlatformType::kWindows);

  // Act
  BrowserManager::Get()->OnActive();
  BrowserManager::Get()->OnForegrounded();

  // Assert
  BrowserIsActiveFrequencyCap frequency_cap;
  const bool is_allowed = frequency_cap.ShouldAllow();
  EXPECT_TRUE(is_allowed);
}

TEST_F(BatAdsBrowserIsActiveFrequencyCapTest, AlwaysAllowAdForAndroid) {
  // Arrange
  MockPlatformHelper(platform_helper_mock_, PlatformType::kAndroid);

  // Act
  BrowserManager::Get()->OnInactive();
  BrowserManager::Get()->OnBackgrounded();

  // Assert
  BrowserIsActiveFrequencyCap frequency_cap;
  const bool is_allowed = frequency_cap.ShouldAllow();
  EXPECT_TRUE(is_allowed);
}

TEST_F(BatAdsBrowserIsActiveFrequencyCapTest, DoNotAllowAd) {
  // Arrange
  MockPlatformHelper(platform_helper_mock_, PlatformType::kWindows);

  // Act
  BrowserManager::Get()->OnInactive();
  BrowserManager::Get()->OnBackgrounded();

  // Assert
  BrowserIsActiveFrequencyCap frequency_cap;
  const bool is_allowed = frequency_cap.ShouldAllow();
  EXPECT_FALSE(is_allowed);
}

TEST_F(BatAdsBrowserIsActiveFrequencyCapTest, AllowAdIfFrequencyCapIsDisabled) {
  // Arrange
  base::FieldTrialParams kParameters;
  kParameters["should_only_serve_ads_if_browser_is_active"] = "false";
  std::vector<base::test::ScopedFeatureList::FeatureAndParams> enabled_features;
  enabled_features.push_back(
      {features::frequency_capping::kFeature, kParameters});

  const std::vector<base::Feature> disabled_features;

  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitWithFeaturesAndParameters(enabled_features,
                                                    disabled_features);

  MockPlatformHelper(platform_helper_mock_, PlatformType::kWindows);

  // Act
  BrowserManager::Get()->OnInactive();
  BrowserManager::Get()->OnBackgrounded();

  // Assert
  BrowserIsActiveFrequencyCap frequency_cap;
  const bool is_allowed = frequency_cap.ShouldAllow();
  EXPECT_TRUE(is_allowed);
}

TEST_F(BatAdsBrowserIsActiveFrequencyCapTest,
       DoNotAllowAdIfWindowIsActiveAndBrowserIsBackgrounded) {
  // Arrange
  MockPlatformHelper(platform_helper_mock_, PlatformType::kWindows);

  // Act
  BrowserManager::Get()->OnActive();
  BrowserManager::Get()->OnBackgrounded();

  // Assert
  BrowserIsActiveFrequencyCap frequency_cap;
  const bool is_allowed = frequency_cap.ShouldAllow();
  EXPECT_FALSE(is_allowed);
}

TEST_F(BatAdsBrowserIsActiveFrequencyCapTest,
       DoNotAllowAdIfWindowIsInactiveAndBrowserIsForegrounded) {
  // Arrange
  MockPlatformHelper(platform_helper_mock_, PlatformType::kWindows);

  // Act
  BrowserManager::Get()->OnInactive();
  BrowserManager::Get()->OnForegrounded();

  // Assert
  BrowserIsActiveFrequencyCap frequency_cap;
  const bool is_allowed = frequency_cap.ShouldAllow();
  EXPECT_FALSE(is_allowed);
}

}  // namespace ads
