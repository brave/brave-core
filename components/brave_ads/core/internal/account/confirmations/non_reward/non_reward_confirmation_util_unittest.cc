/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/account/confirmations/non_reward/non_reward_confirmation_util.h"

#include "brave/components/brave_ads/core/confirmation_type.h"
#include "brave/components/brave_ads/core/internal/account/confirmations/confirmation_info.h"
#include "brave/components/brave_ads/core/internal/account/confirmations/confirmations_util.h"
#include "brave/components/brave_ads/core/internal/account/transactions/transaction_info.h"
#include "brave/components/brave_ads/core/internal/account/transactions/transactions_unittest_util.h"
#include "brave/components/brave_ads/core/internal/ads/ad_unittest_util.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_base.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

class BraveAdsNonRewardConfirmationUtilTest : public UnitTestBase {};

TEST_F(BraveAdsNonRewardConfirmationUtilTest, BuildNonRewardConfirmation) {
  // Arrange
  DisableBraveRewards();

  const TransactionInfo transaction =
      BuildUnreconciledTransaction(/*value*/ 0.1, ConfirmationType::kViewed,
                                   /*should_use_random_uuids*/ true);

  // Act
  const absl::optional<ConfirmationInfo> confirmation =
      BuildNonRewardConfirmation(transaction,
                                 /*user_data*/ {});
  ASSERT_TRUE(confirmation);

  // Assert
  // TODO(tmancey): Check data structure values.
  EXPECT_FALSE(confirmation->reward);
  EXPECT_TRUE(IsValid(*confirmation));
}

}  // namespace brave_ads
