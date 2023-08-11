/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/account/confirmations/non_reward/non_reward_confirmation_util.h"

#include "base/test/bind.h"
#include "brave/components/brave_ads/core/confirmation_type.h"
#include "brave/components/brave_ads/core/internal/account/confirmations/confirmation_info.h"
#include "brave/components/brave_ads/core/internal/account/confirmations/user_data_builder/confirmation_user_data_builder.h"
#include "brave/components/brave_ads/core/internal/account/confirmations/user_data_builder/confirmation_user_data_builder_unittest_util.h"
#include "brave/components/brave_ads/core/internal/account/transactions/transaction_info.h"
#include "brave/components/brave_ads/core/internal/account/transactions/transaction_unittest_constants.h"
#include "brave/components/brave_ads/core/internal/account/transactions/transactions_unittest_util.h"
#include "brave/components/brave_ads/core/internal/ads/ad_unittest_constants.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_base.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_time_util.h"
#include "brave/components/brave_ads/core/internal/settings/settings_unittest_util.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

class BraveAdsNonRewardConfirmationUtilTest : public UnitTestBase {
 protected:
  void SetUp() override {
    UnitTestBase::SetUp();

    DisableBraveRewardsForTesting();

    MockConfirmationUserData();

    AdvanceClockTo(
        TimeFromString("Mon, 8 Jul 1996 09:25:00", /*is_local*/ false));
  }
};

TEST_F(BraveAdsNonRewardConfirmationUtilTest, BuildNonRewardConfirmation) {
  // Arrange
  const TransactionInfo transaction = BuildUnreconciledTransactionForTesting(
      /*value*/ 0.01, ConfirmationType::kViewed,
      /*should_use_random_uuids*/ false);

  // Act
  BuildConfirmationUserData(
      transaction,
      base::BindLambdaForTesting([&transaction](const UserDataInfo& user_data) {
        // Assert
        ConfirmationInfo expected_confirmation;
        expected_confirmation.transaction_id = kTransactionId;
        expected_confirmation.creative_instance_id = kCreativeInstanceId;
        expected_confirmation.type = ConfirmationType::kViewed;
        expected_confirmation.ad_type = AdType::kNotificationAd;
        expected_confirmation.created_at = Now();
        expected_confirmation.was_created = false;

        EXPECT_EQ(expected_confirmation,
                  BuildNonRewardConfirmation(transaction, user_data));
      }));
}

}  // namespace brave_ads
