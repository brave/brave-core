/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/settings/settings.h"

#include <vector>

#include "base/test/scoped_feature_list.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_base.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_pref_util.h"
#include "brave/components/brave_ads/core/internal/settings/settings_unittest_util.h"
#include "brave/components/brave_ads/core/public/feature/notification_ad_feature.h"
#include "brave/components/brave_ads/core/public/prefs/pref_names.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

class BraveAdsSettingsTest : public UnitTestBase {};

TEST_F(BraveAdsSettingsTest, UserHasJoinedBraveRewards) {
  // Arrange

  // Act

  // Assert
  EXPECT_TRUE(UserHasJoinedBraveRewards());
}

TEST_F(BraveAdsSettingsTest, UserHasNotJoinedBraveRewards) {
  // Arrange
  DisableBraveRewardsForTesting();

  // Act

  // Assert
  EXPECT_FALSE(UserHasJoinedBraveRewards());
}

TEST_F(BraveAdsSettingsTest, UserHasOptedInToBraveNewsAds) {
  // Arrange

  // Act

  // Assert
  EXPECT_TRUE(UserHasOptedInToBraveNewsAds());
}

TEST_F(BraveAdsSettingsTest, UserHasNotOptedInToBraveNews) {
  // Arrange
  DisableBraveNewsAdsForTesting();

  // Act

  // Assert
  EXPECT_FALSE(UserHasOptedInToBraveNewsAds());
}

TEST_F(BraveAdsSettingsTest, UserHasOptedInToNewTabPageAds) {
  // Arrange

  // Act

  // Assert
  EXPECT_TRUE(UserHasOptedInToNewTabPageAds());
}

TEST_F(BraveAdsSettingsTest, UserHasNotOptedInToNewTabPageAds) {
  // Arrange
  DisableNewTabPageAdsForTesting();

  // Act

  // Assert
  EXPECT_FALSE(UserHasOptedInToNewTabPageAds());
}

TEST_F(BraveAdsSettingsTest, UserHasOptedInToNotificationAds) {
  // Arrange

  // Act

  // Assert
  EXPECT_TRUE(UserHasOptedInToNotificationAds());
}

TEST_F(BraveAdsSettingsTest, UserHasNotOptedInToNotificationAds) {
  // Arrange
  DisableNotificationAdsForTesting();

  // Act

  // Assert
  EXPECT_FALSE(UserHasOptedInToNotificationAds());
}

TEST_F(BraveAdsSettingsTest, MaximumNotificationAdsPerHour) {
  // Arrange
  std::vector<base::test::FeatureRefAndParams> enabled_features;
  base::FieldTrialParams params;
  params["default_ads_per_hour"] = "2";
  enabled_features.emplace_back(kNotificationAdFeature, params);

  const std::vector<base::test::FeatureRef> disabled_features;

  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitWithFeaturesAndParameters(enabled_features,
                                                    disabled_features);

  SetInt64Pref(prefs::kMaximumNotificationAdsPerHour, 3);

  // Act

  // Assert
  EXPECT_EQ(3, GetMaximumNotificationAdsPerHour());
}

TEST_F(BraveAdsSettingsTest, DefaultMaximumNotificationAdsPerHour) {
  // Arrange
  std::vector<base::test::FeatureRefAndParams> enabled_features;
  base::FieldTrialParams params;
  params["default_ads_per_hour"] = "2";
  enabled_features.emplace_back(kNotificationAdFeature, params);

  const std::vector<base::test::FeatureRef> disabled_features;

  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitWithFeaturesAndParameters(enabled_features,
                                                    disabled_features);

  // Act

  // Assert
  EXPECT_EQ(2, GetMaximumNotificationAdsPerHour());
}

}  // namespace brave_ads
