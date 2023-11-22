/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/settings/settings.h"

#include "base/test/scoped_feature_list.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_base.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_profile_pref_value.h"
#include "brave/components/brave_ads/core/internal/settings/settings_unittest_util.h"
#include "brave/components/brave_ads/core/public/prefs/pref_names.h"
#include "brave/components/brave_ads/core/public/units/notification_ad/notification_ad_feature.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

class BraveAdsSettingsTest : public UnitTestBase {};

TEST_F(BraveAdsSettingsTest, UserHasJoinedBraveRewards) {
  // Act & Assert
  EXPECT_TRUE(UserHasJoinedBraveRewards());
}

TEST_F(BraveAdsSettingsTest, UserHasNotJoinedBraveRewards) {
  // Arrange
  test::DisableBraveRewards();

  // Act & Assert
  EXPECT_FALSE(UserHasJoinedBraveRewards());
}

TEST_F(BraveAdsSettingsTest, UserHasOptedInToBraveNewsAds) {
  // Act & Assert
  EXPECT_TRUE(UserHasOptedInToBraveNewsAds());
}

TEST_F(BraveAdsSettingsTest, UserHasNotOptedInToBraveNews) {
  // Arrange
  test::OptOutOfBraveNewsAds();

  // Act & Assert
  EXPECT_FALSE(UserHasOptedInToBraveNewsAds());
}

TEST_F(BraveAdsSettingsTest, UserHasOptedInToNewTabPageAds) {
  // Act & Assert
  EXPECT_TRUE(UserHasOptedInToNewTabPageAds());
}

TEST_F(BraveAdsSettingsTest, UserHasNotOptedInToNewTabPageAds) {
  // Arrange
  test::OptOutOfNewTabPageAds();

  // Act & Assert
  EXPECT_FALSE(UserHasOptedInToNewTabPageAds());
}

TEST_F(BraveAdsSettingsTest, UserHasOptedInToNotificationAds) {
  // Act & Assert
  EXPECT_TRUE(UserHasOptedInToNotificationAds());
}

TEST_F(BraveAdsSettingsTest, UserHasNotOptedInToNotificationAds) {
  // Arrange
  test::OptOutOfNotificationAds();

  // Act & Assert
  EXPECT_FALSE(UserHasOptedInToNotificationAds());
}

TEST_F(BraveAdsSettingsTest, MaximumNotificationAdsPerHour) {
  // Arrange
  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitAndEnableFeatureWithParameters(
      kNotificationAdFeature, {{"default_ads_per_hour", "2"}});

  SetProfileInt64PrefValue(prefs::kMaximumNotificationAdsPerHour, 3);

  // Act & Assert
  EXPECT_EQ(3, GetMaximumNotificationAdsPerHour());
}

TEST_F(BraveAdsSettingsTest, DefaultMaximumNotificationAdsPerHour) {
  // Arrange
  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitAndEnableFeatureWithParameters(
      kNotificationAdFeature, {{"default_ads_per_hour", "2"}});

  // Act & Assert
  EXPECT_EQ(2, GetMaximumNotificationAdsPerHour());
}

}  // namespace brave_ads
