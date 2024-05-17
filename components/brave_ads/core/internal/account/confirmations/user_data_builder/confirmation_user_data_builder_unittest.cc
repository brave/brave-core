/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/account/confirmations/user_data_builder/confirmation_user_data_builder.h"

#include <string>
#include <utility>

#include "base/test/values_test_util.h"
#include "base/values.h"
#include "brave/components/brave_ads/core/internal/account/confirmations/user_data_builder/confirmation_user_data_builder_unittest_util.h"
#include "brave/components/brave_ads/core/internal/account/transactions/transaction_info.h"
#include "brave/components/brave_ads/core/internal/account/transactions/transactions_unittest_util.h"
#include "brave/components/brave_ads/core/internal/account/user_data/user_data_info.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_base.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_time_converter_util.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_time_util.h"
#include "brave/components/brave_ads/core/internal/settings/settings_unittest_util.h"
#include "brave/components/brave_ads/core/public/account/confirmations/confirmation_type.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

class BraveAdsConfirmationUserDataBuilderTest : public UnitTestBase {
 protected:
  void SetUp() override {
    UnitTestBase::SetUp();

    MockConfirmationUserData();

    AdvanceClockTo(TimeFromUTCString("November 18 2020 12:34:56.789"));
  }
};

TEST_F(BraveAdsConfirmationUserDataBuilderTest,
       BuildConfirmationUserDataForRewardsUser) {
  // Arrange
  const TransactionInfo transaction = test::BuildTransaction(
      /*value=*/0.01, AdType::kNotificationAd,
      ConfirmationType::kViewedImpression, /*reconciled_at=*/Now(),
      /*should_use_random_uuids=*/false);

  // Act & Assert
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
            "countryCode": "US",
            "createdAtTimestamp": "2020-11-18T12:00:00.000Z",
            "foo": "bar",
            "platform": "windows",
            "rotating_hash": "I6KM54gXOrWqRHyrD518LmhePLHpIk4KSgCKOl0e3sc=",
            "segment": "untargeted",
            "studies": [],
            "topSegment": [],
            "versionNumber": "1.2.3.4"
          })");

  auto user_data = base::Value::Dict().Set("foo", "bar");

  EXPECT_EQ(expected_user_data,
            BuildConfirmationUserData(transaction, std::move(user_data)));
}

TEST_F(BraveAdsConfirmationUserDataBuilderTest,
       BuildConfirmationUserDataForNonRewardsUser) {
  // Arrange
  test::DisableBraveRewards();

  const TransactionInfo transaction = test::BuildTransaction(
      /*value=*/0.01, AdType::kNotificationAd,
      ConfirmationType::kViewedImpression, /*reconciled_at=*/Now(),
      /*should_use_random_uuids=*/false);

  UserDataInfo expected_user_data;
  expected_user_data.fixed = base::test::ParseJsonDict(
      R"(
          {
            "foo": "bar"
          })");

  auto user_data = base::Value::Dict().Set("foo", "bar");

  // Act & Assert
  EXPECT_EQ(expected_user_data,
            BuildConfirmationUserData(transaction, std::move(user_data)));
}

}  // namespace brave_ads
