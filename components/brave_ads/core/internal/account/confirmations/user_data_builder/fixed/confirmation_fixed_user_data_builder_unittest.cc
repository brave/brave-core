/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/account/confirmations/user_data_builder/fixed/confirmation_fixed_user_data_builder.h"

#include "base/test/values_test_util.h"
#include "base/values.h"
#include "brave/components/brave_ads/core/internal/account/confirmations/user_data_builder/confirmation_user_data_builder_unittest_util.h"
#include "brave/components/brave_ads/core/internal/account/transactions/transaction_info.h"
#include "brave/components/brave_ads/core/internal/account/transactions/transactions_unittest_util.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_base.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_time_converter_util.h"
#include "brave/components/brave_ads/core/internal/settings/settings_unittest_util.h"
#include "brave/components/brave_ads/core/public/account/confirmations/confirmation_type.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

class BraveAdsFixedUserDataBuilderTest : public UnitTestBase {
 protected:
  void SetUp() override {
    UnitTestBase::SetUp();

    MockConfirmationUserData();

    AdvanceClockTo(TimeFromUTCString("November 18 2020 12:34:56.789"));
  }
};

TEST_F(BraveAdsFixedUserDataBuilderTest, BuildFixedUserDataForRewardsUser) {
  // Arrange
  const TransactionInfo transaction = test::BuildUnreconciledTransaction(
      /*value=*/0.01, AdType::kNotificationAd,
      ConfirmationType::kViewedImpression, /*should_use_random_uuids=*/false);

  // Act & Assert
  const base::Value::Dict expected_user_data = base::test::ParseJsonDict(
      R"(
          {
            "buildChannel": "release",
            "catalog": [
              {
                "id": "29e5c8bc0ba319069980bb390d8e8f9b58c05a20"
              }
            ],
            "countryCode": "US",
            "createdAtTimestamp": "2020-11-18T12:00:00.000Z",
            "platform": "windows",
            "rotating_hash": "I6KM54gXOrWqRHyrD518LmhePLHpIk4KSgCKOl0e3sc=",
            "segment": "untargeted",
            "studies": [],
            "topSegment": [],
            "versionNumber": "1.2.3.4"
          })");
  EXPECT_EQ(expected_user_data, BuildFixedUserData(transaction));
}

TEST_F(BraveAdsFixedUserDataBuilderTest, BuildFixedUserDataForNonRewardsUser) {
  // Arrange
  test::DisableBraveRewards();

  const TransactionInfo transaction = test::BuildUnreconciledTransaction(
      /*value=*/0.01, AdType::kNotificationAd,
      ConfirmationType::kViewedImpression, /*should_use_random_uuids=*/false);

  // Act
  const base::Value::Dict user_data = BuildFixedUserData(transaction);

  // Assert
  EXPECT_TRUE(user_data.empty());
}

}  // namespace brave_ads
