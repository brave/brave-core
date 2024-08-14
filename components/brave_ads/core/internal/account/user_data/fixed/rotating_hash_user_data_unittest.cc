/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/account/user_data/fixed/rotating_hash_user_data.h"

#include "base/test/values_test_util.h"
#include "brave/components/brave_ads/core/internal/account/transactions/transaction_info.h"
#include "brave/components/brave_ads/core/internal/account/transactions/transactions_test_util.h"
#include "brave/components/brave_ads/core/internal/common/test/mock_test_util.h"
#include "brave/components/brave_ads/core/internal/common/test/test_base.h"
#include "brave/components/brave_ads/core/internal/common/test/time_test_util.h"
#include "brave/components/brave_ads/core/internal/settings/settings_test_util.h"
#include "brave/components/brave_ads/core/public/account/confirmations/confirmation_type.h"
#include "brave/components/brave_ads/core/public/ad_units/ad_type.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

class BraveAdsRotatingHashUserDataTest : public test::TestBase {
 protected:
  void SetUp() override {
    test::TestBase::SetUp();

    AdvanceClockTo(test::TimeFromUTCString("2 June 2022 11:00"));
  }
};

TEST_F(BraveAdsRotatingHashUserDataTest,
       BuildRotatingHashUserDataForRewardsUser) {
  // Arrange
  test::MockDeviceId();

  const TransactionInfo transaction = test::BuildUnreconciledTransaction(
      /*value=*/0.01, AdType::kNotificationAd,
      ConfirmationType::kViewedImpression,
      /*should_generate_random_uuids=*/false);

  // Act
  const base::Value::Dict user_data = BuildRotatingHashUserData(transaction);

  // Assert
  EXPECT_EQ(base::test::ParseJsonDict(
                R"(
                    {
                      "rotatingHash": "j9D7eKSoPLYNfxkG2Mx+SbgKJ9hcKg1QwDB8B5qxlpk="
                    })"),
            user_data);
}

TEST_F(BraveAdsRotatingHashUserDataTest,
       DoNotBuildRotatingHashUserDataForNonRewardsUser) {
  // Arrange
  test::DisableBraveRewards();

  test::MockDeviceId();

  const TransactionInfo transaction = test::BuildUnreconciledTransaction(
      /*value=*/0.01, AdType::kNotificationAd,
      ConfirmationType::kViewedImpression,
      /*should_generate_random_uuids=*/false);

  // Act
  const base::Value::Dict user_data = BuildRotatingHashUserData(transaction);

  // Assert
  EXPECT_THAT(user_data, ::testing::IsEmpty());
}

TEST_F(BraveAdsRotatingHashUserDataTest,
       DoNotBuildRotatingHashUserDataIfMissingDeviceId) {
  // Arrange
  const TransactionInfo transaction = test::BuildUnreconciledTransaction(
      /*value=*/0.01, AdType::kNotificationAd,
      ConfirmationType::kViewedImpression,
      /*should_generate_random_uuids=*/false);

  // Act
  const base::Value::Dict user_data = BuildRotatingHashUserData(transaction);

  // Assert
  EXPECT_THAT(user_data, ::testing::IsEmpty());
}

}  // namespace brave_ads
