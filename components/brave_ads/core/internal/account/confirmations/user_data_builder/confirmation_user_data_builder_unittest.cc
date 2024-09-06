/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/account/confirmations/user_data_builder/confirmation_user_data_builder.h"

#include <string>

#include "base/test/values_test_util.h"
#include "base/values.h"
#include "brave/components/brave_ads/core/internal/account/confirmations/user_data_builder/confirmation_user_data_builder_test_util.h"
#include "brave/components/brave_ads/core/internal/account/transactions/transaction_info.h"
#include "brave/components/brave_ads/core/internal/account/transactions/transactions_test_util.h"
#include "brave/components/brave_ads/core/internal/account/user_data/user_data_info.h"
#include "brave/components/brave_ads/core/internal/common/test/test_base.h"
#include "brave/components/brave_ads/core/internal/common/test/time_test_util.h"
#include "brave/components/brave_ads/core/internal/settings/settings_test_util.h"
#include "brave/components/brave_ads/core/mojom/brave_ads.mojom.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

class BraveAdsConfirmationUserDataBuilderTest : public test::TestBase {
 protected:
  void SetUp() override {
    test::TestBase::SetUp();

    test::MockConfirmationUserData();

    AdvanceClockTo(test::TimeFromUTCString("November 18 2020 12:34:56.789"));
  }
};

TEST_F(BraveAdsConfirmationUserDataBuilderTest,
       BuildConfirmationUserDataForRewardsUser) {
  // Arrange
  const TransactionInfo transaction = test::BuildTransaction(
      /*value=*/0.01, mojom::AdType::kNotificationAd,
      mojom::ConfirmationType::kViewedImpression, /*reconciled_at=*/test::Now(),
      /*should_generate_random_uuids=*/false);

  // Act
  const UserDataInfo user_data = BuildConfirmationUserData(
      transaction,
      /*user_data=*/base::Value::Dict().Set("foo", "bar"));

  // Assert
  UserDataInfo expected_user_data;
  expected_user_data.dynamic = base::test::ParseJsonDict(
      R"(
          {
            "diagnosticId": "c1298fde-7fdb-401f-a3ce-0b58fe86e6e2",
            "systemTimestamp": "2020-11-18T12:00:00.000Z"
          })");
  expected_user_data.fixed = base::test::ParseJsonDict(
      R"(
          {
            "buildChannel": "release",
            "catalog": [
              {
                "id": "29e5c8bc0ba319069980bb390d8e8f9b58c05a20"
              }
            ],
            "createdAtTimestamp": "2020-11-18T12:00:00.000Z",
            "foo": "bar",
            "platform": "windows",
            "rotatingHash": "I6KM54gXOrWqRHyrD518LmhePLHpIk4KSgCKOl0e3sc=",
            "segment": "untargeted",
            "studies": [],
            "versionNumber": "1.2.3.4"
          })");

  EXPECT_EQ(expected_user_data, user_data);
}

TEST_F(BraveAdsConfirmationUserDataBuilderTest,
       BuildConfirmationUserDataForNonRewardsUser) {
  // Arrange
  test::DisableBraveRewards();

  const TransactionInfo transaction = test::BuildTransaction(
      /*value=*/0.01, mojom::AdType::kNotificationAd,
      mojom::ConfirmationType::kViewedImpression, /*reconciled_at=*/test::Now(),
      /*should_generate_random_uuids=*/false);

  // Act
  const UserDataInfo user_data = BuildConfirmationUserData(
      transaction,
      /*user_data=*/base::Value::Dict().Set("foo", "bar"));

  // Assert
  UserDataInfo expected_user_data;
  expected_user_data.fixed = base::test::ParseJsonDict(
      R"(
          {
            "foo": "bar"
          })");

  EXPECT_EQ(expected_user_data, user_data);
}

}  // namespace brave_ads
