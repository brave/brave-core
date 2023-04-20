/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/account/deposits/non_cash_deposit.h"

#include "base/functional/bind.h"
#include "brave/components/brave_ads/core/internal/ads/ad_unittest_constants.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_base.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

class BraveAdsNonCashDepositTest : public UnitTestBase {};

TEST_F(BraveAdsNonCashDepositTest, GetValue) {
  // Arrange
  NonCashDeposit deposit;

  // Act
  deposit.GetValue(kCreativeInstanceId,
                   base::BindOnce([](const bool success, const double value) {
                     EXPECT_TRUE(success);
                     EXPECT_EQ(0.0, value);
                   }));

  // Assert
}

}  // namespace brave_ads
