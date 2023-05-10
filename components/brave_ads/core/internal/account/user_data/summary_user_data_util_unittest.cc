/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/account/user_data/summary_user_data_util.h"

#include "brave/components/brave_ads/core/internal/privacy/tokens/unblinded_payment_tokens/unblinded_payment_tokens_unittest_util.h"
#include "testing/gtest/include/gtest/gtest.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

TEST(BraveAdsSummaryUserDataUtilTest, BuildBucketsIfNoUnblindedPaymentTokens) {
  // Arrange

  // Act
  const AdTypeBucketMap buckets = BuildBuckets(/*unblinded_payment_tokens*/ {});

  // Assert
  EXPECT_TRUE(buckets.empty());
}

TEST(BraveAdsSummaryUserDataUtilTest, BuildBuckets) {
  // Arrange

  // Act

  // Assert
  const AdTypeBucketMap expected_buckets = {{"ad_notification", {{"view", 2}}}};
  EXPECT_EQ(expected_buckets,
            BuildBuckets(privacy::BuildUnblindedPaymentTokens(/*count*/ 2)));
}

}  // namespace brave_ads
