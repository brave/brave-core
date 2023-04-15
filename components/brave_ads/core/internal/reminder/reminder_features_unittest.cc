/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/reminder/reminder_features.h"

#include <vector>

#include "base/test/scoped_feature_list.h"
#include "testing/gtest/include/gtest/gtest.h"

// npm run test -- brave_unit_tests --filter=BatAds*

namespace brave_ads::features {

TEST(BatAdsReminderFeaturesTest, IsEnabled) {
  // Arrange

  // Act

  // Assert
  EXPECT_TRUE(IsEnabled());
}
TEST(BatAdsReminderFeaturesTest, IsDisabled) {
  // Arrange
  const std::vector<base::test::FeatureRefAndParams> enabled_features;

  std::vector<base::test::FeatureRef> disabled_features;
  disabled_features.emplace_back(kReminder);

  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitWithFeaturesAndParameters(enabled_features,
                                                    disabled_features);

  // Act

  // Assert
  EXPECT_FALSE(IsEnabled());
}

TEST(BatAdsReminderFeaturesTest, RemindUserIfClickingTheSameAdAfter) {
  // Arrange
  base::FieldTrialParams params;
  params["remind_user_if_clicking_the_same_ad_after"] = "1";
  std::vector<base::test::FeatureRefAndParams> enabled_features;
  enabled_features.emplace_back(kReminder, params);

  const std::vector<base::test::FeatureRef> disabled_features;

  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitWithFeaturesAndParameters(enabled_features,
                                                    disabled_features);

  // Act

  // Assert
  EXPECT_EQ(1U, GetRemindUserIfClickingTheSameAdAfter());
}

TEST(BatAdsReminderFeaturesTest, DefaultRemindUserIfClickingTheSameAdAfter) {
  // Arrange

  // Act

  // Assert
  EXPECT_EQ(3U, GetRemindUserIfClickingTheSameAdAfter());
}

TEST(BatAdsReminderFeaturesTest,
     DefaultRemindUserIfClickingTheSameAdAfterWhenDisabled) {
  // Arrange
  const std::vector<base::test::FeatureRefAndParams> enabled_features;

  std::vector<base::test::FeatureRef> disabled_features;
  disabled_features.emplace_back(kReminder);

  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitWithFeaturesAndParameters(enabled_features,
                                                    disabled_features);

  // Act

  // Assert
  EXPECT_EQ(3U, GetRemindUserIfClickingTheSameAdAfter());
}

}  // namespace brave_ads::features
