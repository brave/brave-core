/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/account/utility/redeem_payment_tokens/redeem_payment_tokens_util.h"

#include "brave/components/brave_ads/core/internal/ads_client/ads_client_util.h"
#include "brave/components/brave_ads/core/internal/common/test/test_base.h"
#include "brave/components/brave_ads/core/internal/common/test/time_test_util.h"
#include "brave/components/brave_ads/core/public/prefs/pref_names.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

class BraveAdsRedeemPaymentTokensUtilTest : public test::TestBase {};

TEST_F(BraveAdsRedeemPaymentTokensUtilTest, SetNextTokenRedemptionAt) {
  // Arrange
  SetNextTokenRedemptionAt(test::DistantFuture());

  // Act & Assert
  EXPECT_FALSE(GetProfileTimePref(prefs::kNextTokenRedemptionAt).is_null());
}

TEST_F(BraveAdsRedeemPaymentTokensUtilTest, ScheduleNextTokenRedemptionAt) {
  // Act & Assert
  EXPECT_FALSE(ScheduleNextTokenRedemptionAt().is_null());
}

TEST_F(BraveAdsRedeemPaymentTokensUtilTest,
       CalculateDelayBeforeRedeemingTokens) {
  // Arrange
  SetNextTokenRedemptionAt(test::Now() + base::Days(1));

  // Act & Assert
  EXPECT_EQ(base::Days(1), CalculateDelayBeforeRedeemingTokens());
}

TEST_F(BraveAdsRedeemPaymentTokensUtilTest,
       CalculateDelayBeforeRedeemingTokensIfHasNotPreviouslyRedeemedTokens) {
  // Act & Assert
  EXPECT_FALSE(ScheduleNextTokenRedemptionAt().is_null());
}

TEST_F(BraveAdsRedeemPaymentTokensUtilTest,
       CalculateDelayBeforeRedeemingTokensIfShouldHaveRedeemedTokensInThePast) {
  // Arrange
  SetNextTokenRedemptionAt(test::DistantPast());

  // Act & Assert
  EXPECT_EQ(base::Minutes(1), CalculateDelayBeforeRedeemingTokens());
}

TEST_F(BraveAdsRedeemPaymentTokensUtilTest,
       CalculateMinimumDelayBeforeRedeemingTokens) {
  // Arrange
  SetNextTokenRedemptionAt(test::Now() + base::Milliseconds(1));

  // Act & Assert
  EXPECT_EQ(base::Minutes(1), CalculateDelayBeforeRedeemingTokens());
}

}  // namespace brave_ads
