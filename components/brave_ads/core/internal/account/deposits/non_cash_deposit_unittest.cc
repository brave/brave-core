/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/account/deposits/non_cash_deposit.h"

#include "base/test/mock_callback.h"
#include "brave/components/brave_ads/core/internal/ad_units/ad_unittest_constants.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_base.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

class BraveAdsNonCashDepositTest : public UnitTestBase {};

TEST_F(BraveAdsNonCashDepositTest, GetValue) {
  // Arrange
  NonCashDeposit deposit;

  // Act & Assert
  base::MockCallback<GetDepositCallback> callback;
  EXPECT_CALL(callback, Run(/*success=*/true, /*value=*/0.0));
  deposit.GetValue(kCreativeInstanceId, callback.Get());
}

}  // namespace brave_ads
