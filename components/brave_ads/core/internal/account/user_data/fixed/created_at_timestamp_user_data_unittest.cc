/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/account/user_data/fixed/created_at_timestamp_user_data.h"

#include "base/test/values_test_util.h"
#include "brave/components/brave_ads/core/internal/account/transactions/transaction_info.h"
#include "brave/components/brave_ads/core/internal/account/transactions/transactions_test_util.h"
#include "brave/components/brave_ads/core/internal/common/test/test_base.h"
#include "brave/components/brave_ads/core/internal/common/test/time_test_util.h"
#include "brave/components/brave_ads/core/internal/settings/settings_test_util.h"
#include "brave/components/brave_ads/core/public/account/confirmations/confirmation_type.h"
#include "brave/components/brave_ads/core/public/ad_units/ad_type.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

class BraveAdsCreatedAtTimestampUserDataTest : public test::TestBase {
 protected:
  void SetUp() override {
    test::TestBase::SetUp();

    AdvanceClockTo(test::TimeFromUTCString("November 18 2020 12:34:56.789"));
  }
};

TEST_F(BraveAdsCreatedAtTimestampUserDataTest,
       BuildCreatedAtTimestampUserDataForRewardsUser) {
  // Arrange
  const TransactionInfo transaction = test::BuildUnreconciledTransaction(
      /*value=*/0.01, AdType::kNotificationAd,
      ConfirmationType::kViewedImpression,
      /*should_generate_random_uuids=*/true);

  // Act
  const base::Value::Dict user_data =
      BuildCreatedAtTimestampUserData(transaction);

  // Assert
  EXPECT_EQ(base::test::ParseJsonDict(
                R"(
                    {
                      "createdAtTimestamp": "2020-11-18T12:00:00.000Z"
                    })"),
            user_data);
}

TEST_F(BraveAdsCreatedAtTimestampUserDataTest,
       BuildCreatedAtTimestampUserDataForNonRewardsUser) {
  // Arrange
  test::DisableBraveRewards();

  const TransactionInfo transaction = test::BuildUnreconciledTransaction(
      /*value=*/0.01, AdType::kNotificationAd,
      ConfirmationType::kViewedImpression,
      /*should_generate_random_uuids=*/true);

  // Act
  const base::Value::Dict user_data =
      BuildCreatedAtTimestampUserData(transaction);

  // Assert
  EXPECT_THAT(user_data, ::testing::IsEmpty());
}

}  // namespace brave_ads
