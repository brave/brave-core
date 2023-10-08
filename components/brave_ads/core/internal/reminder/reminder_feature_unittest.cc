/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/reminder/reminder_feature.h"

#include "base/test/scoped_feature_list.h"
#include "testing/gtest/include/gtest/gtest.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

TEST(BraveAdsReminderFeatureTest, IsEnabled) {
  // Act & Assert
  EXPECT_TRUE(base::FeatureList::IsEnabled(kReminderFeature));
}

TEST(BraveAdsReminderFeatureTest, IsDisabled) {
  // Arrange
  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitAndDisableFeature(kReminderFeature);

  // Act & Assert
  EXPECT_FALSE(base::FeatureList::IsEnabled(kReminderFeature));
}

TEST(BraveAdsReminderFeatureTest, RemindUserIfClickingTheSameAdAfter) {
  // Arrange
  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitAndEnableFeatureWithParameters(
      kReminderFeature, {{"remind_user_if_clicking_the_same_ad_after", "1"}});

  // Act & Assert
  EXPECT_EQ(1, kRemindUserIfClickingTheSameAdAfter.Get());
}

TEST(BraveAdsReminderFeatureTest, DefaultRemindUserIfClickingTheSameAdAfter) {
  // Act & Assert
  EXPECT_EQ(3, kRemindUserIfClickingTheSameAdAfter.Get());
}

TEST(BraveAdsReminderFeatureTest,
     DefaultRemindUserIfClickingTheSameAdAfterWhenDisabled) {
  // Arrange
  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitAndDisableFeature(kReminderFeature);

  // Act & Assert
  EXPECT_EQ(3, kRemindUserIfClickingTheSameAdAfter.Get());
}

}  // namespace brave_ads
