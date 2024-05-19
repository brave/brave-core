/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/account/user_data/fixed/rotating_hash_user_data_util.h"

#include "brave/components/brave_ads/core/internal/account/transactions/transaction_info.h"
#include "brave/components/brave_ads/core/internal/account/transactions/transactions_unittest_util.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_base.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_mock_util.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_time_converter_util.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

class BraveAdsRotatingHashUserDataUtilTest : public UnitTestBase {};

TEST_F(BraveAdsRotatingHashUserDataUtilTest,
       DoNotBuildRotatingHashIfMissingDeviceId) {
  // Arrange
  AdvanceClockTo(TimeFromUTCString("2 June 2022 11:00"));

  const TransactionInfo transaction = test::BuildUnreconciledTransaction(
      /*value=*/0.01, AdType::kNotificationAd,
      ConfirmationType::kViewedImpression, /*should_use_random_uuids=*/false);

  // Act & Assert
  EXPECT_FALSE(BuildRotatingHash(transaction));
}

TEST_F(BraveAdsRotatingHashUserDataUtilTest, BuildRotatingHash) {
  // Arrange
  MockDeviceId();

  AdvanceClockTo(TimeFromUTCString("2 June 2022 11:00"));

  const TransactionInfo transaction = test::BuildUnreconciledTransaction(
      /*value=*/0.01, AdType::kNotificationAd,
      ConfirmationType::kViewedImpression, /*should_use_random_uuids=*/false);

  // Act & Assert
  EXPECT_EQ("j9D7eKSoPLYNfxkG2Mx+SbgKJ9hcKg1QwDB8B5qxlpk=",
            BuildRotatingHash(transaction));
}

TEST_F(BraveAdsRotatingHashUserDataUtilTest,
       BuildRotatingHashIfWithinSameHour) {
  // Arrange
  MockDeviceId();

  const TransactionInfo transaction = test::BuildUnreconciledTransaction(
      /*value=*/0.01, AdType::kNotificationAd,
      ConfirmationType::kViewedImpression, /*should_use_random_uuids=*/false);

  AdvanceClockTo(TimeFromUTCString("2 June 2022 11:00"));

  const std::optional<std::string> rotating_hash_before =
      BuildRotatingHash(transaction);
  EXPECT_TRUE(rotating_hash_before);

  AdvanceClockBy(base::Hours(1) - base::Milliseconds(1));

  // Act & Assert
  EXPECT_EQ(rotating_hash_before, BuildRotatingHash(transaction));
}

TEST_F(BraveAdsRotatingHashUserDataUtilTest,
       BuildRotatingHashForDifferentHours) {
  // Arrange
  MockDeviceId();

  const TransactionInfo transaction = test::BuildUnreconciledTransaction(
      /*value=*/0.01, AdType::kNotificationAd,
      ConfirmationType::kViewedImpression, /*should_use_random_uuids=*/false);

  AdvanceClockTo(TimeFromUTCString("2 June 2022 11:00"));

  const std::optional<std::string> rotating_hash_before =
      BuildRotatingHash(transaction);
  EXPECT_TRUE(rotating_hash_before);

  AdvanceClockBy(base::Hours(1));

  // Act & Assert
  EXPECT_NE(rotating_hash_before, BuildRotatingHash(transaction));
}

TEST_F(BraveAdsRotatingHashUserDataUtilTest,
       BuildRotatingHashForSameHourButDifferentDay) {
  // Arrange
  MockDeviceId();

  const TransactionInfo transaction = test::BuildUnreconciledTransaction(
      /*value=*/0.01, AdType::kNotificationAd,
      ConfirmationType::kViewedImpression, /*should_use_random_uuids=*/false);

  const std::optional<std::string> rotating_hash_before =
      BuildRotatingHash(transaction);
  EXPECT_TRUE(rotating_hash_before);

  AdvanceClockBy(base::Days(1));

  // Act & Assert
  EXPECT_NE(rotating_hash_before, BuildRotatingHash(transaction));
}

}  // namespace brave_ads
