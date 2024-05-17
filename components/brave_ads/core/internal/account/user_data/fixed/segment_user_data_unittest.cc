/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/account/user_data/fixed/segment_user_data.h"

#include "base/test/values_test_util.h"
#include "brave/components/brave_ads/core/internal/account/transactions/transaction_info.h"
#include "brave/components/brave_ads/core/internal/account/transactions/transactions_unittest_util.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_base.h"
#include "brave/components/brave_ads/core/internal/settings/settings_unittest_util.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

class BraveAdsSegmentUserDataTest : public UnitTestBase {};

TEST_F(BraveAdsSegmentUserDataTest, BuildSegmentUserDataForRewardsUser) {
  // Arrange
  const TransactionInfo transaction = test::BuildUnreconciledTransaction(
      /*value=*/0.01, AdType::kNotificationAd,
      ConfirmationType::kViewedImpression, /*should_use_random_uuids=*/false);

  // Act & Assert
  EXPECT_EQ(base::test::ParseJsonDict(
                R"(
                    {
                      "segment": "untargeted"
                    })"),
            BuildSegmentUserData(transaction));
}

TEST_F(BraveAdsSegmentUserDataTest,
       DoNotBuildSearchResultAdSegmentUserDataForRewardsUser) {
  // Arrange
  const TransactionInfo transaction = test::BuildUnreconciledTransaction(
      /*value=*/0.01, AdType::kSearchResultAd,
      ConfirmationType::kViewedImpression, /*should_use_random_uuids=*/false);

  // Act
  const base::Value::Dict segment_user_data = BuildSegmentUserData(transaction);

  // Assert
  EXPECT_TRUE(segment_user_data.empty());
}

TEST_F(BraveAdsSegmentUserDataTest,
       DoNotBuildSegmentUserDataForNonRewardsUser) {
  // Arrange
  test::DisableBraveRewards();

  const TransactionInfo transaction = test::BuildUnreconciledTransaction(
      /*value=*/0.01, AdType::kNotificationAd,
      ConfirmationType::kViewedImpression, /*should_use_random_uuids=*/false);

  // Act
  const base::Value::Dict segment_user_data = BuildSegmentUserData(transaction);

  // Assert
  EXPECT_TRUE(segment_user_data.empty());
}

TEST_F(BraveAdsSegmentUserDataTest, DoNotBuildSegmentUserDataIfNoTargeting) {
  // Arrange
  const TransactionInfo transaction;

  // Act
  const base::Value::Dict segment_user_data = BuildSegmentUserData(transaction);

  // Act & Assert
  EXPECT_TRUE(segment_user_data.empty());
}

}  // namespace brave_ads
