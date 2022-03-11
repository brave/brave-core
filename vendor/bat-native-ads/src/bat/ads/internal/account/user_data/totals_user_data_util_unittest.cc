/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/account/user_data/totals_user_data_util.h"

#include <string>

#include "base/json/json_writer.h"
#include "bat/ads/internal/privacy/unblinded_payment_tokens/unblinded_payment_tokens_unittest_util.h"
#include "testing/gtest/include/gtest/gtest.h"

// npm run test -- brave_unit_tests --filter=BatAds*

namespace ads {

TEST(BatAdsTotalsUserDataUtilTest, GetBucketsForNoUnblindedPaymentTokens) {
  // Arrange
  const privacy::UnblindedPaymentTokenList unblinded_payment_tokens;

  // Act
  const user_data::AdTypeBucketMap buckets =
      user_data::BuildBuckets(unblinded_payment_tokens);

  // Assert
  const user_data::AdTypeBucketMap expected_buckets;

  EXPECT_EQ(expected_buckets, buckets);
}

TEST(BatAdsTotalsUserDataUtilTest, GetBuckets) {
  // Arrange
  const privacy::UnblindedPaymentTokenList unblinded_payment_tokens =
      privacy::GetUnblindedPaymentTokens(2);

  // Act
  const user_data::AdTypeBucketMap buckets =
      user_data::BuildBuckets(unblinded_payment_tokens);

  // Assert
  const user_data::AdTypeBucketMap expected_buckets = {
      {"ad_notification", {{"view", 2}}}};

  EXPECT_EQ(expected_buckets, buckets);
}

}  // namespace ads
