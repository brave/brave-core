/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/reminders/reminders_feature.h"

#include "testing/gtest/include/gtest/gtest.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

TEST(BraveAdsRemindersFeatureTest, IsEnabled) {
  // Act & Assert
  EXPECT_TRUE(base::FeatureList::IsEnabled(kRemindersFeature));
}

TEST(BraveAdsRemindersFeatureTest, RemindUserIfClickingTheSameAdAfter) {
  // Act & Assert
  EXPECT_EQ(3U, kRemindUserIfClickingTheSameAdAfter.Get());
}

}  // namespace brave_ads
