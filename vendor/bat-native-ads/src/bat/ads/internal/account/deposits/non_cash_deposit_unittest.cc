/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/account/deposits/non_cash_deposit.h"

#include "bat/ads/internal/base/unittest_base.h"
#include "bat/ads/internal/base/unittest_util.h"

// npm run test -- brave_unit_tests --filter=BatAds*

namespace ads {

namespace {
constexpr char kCreativeInstanceId[] = "b77e16fd-e4bf-4bfb-b033-b8772ec6113b";
}  // namespace

class BatAdsNonCashDepositTest : public UnitTestBase {
 protected:
  BatAdsNonCashDepositTest() = default;

  ~BatAdsNonCashDepositTest() override = default;
};

TEST_F(BatAdsNonCashDepositTest, GetValue) {
  // Arrange
  NonCashDeposit deposit;

  // Act
  deposit.GetValue(kCreativeInstanceId,
                   [=](const bool success, const double value) {
                     EXPECT_TRUE(success);
                     EXPECT_EQ(0.0, value);
                   });

  // Assert
}

}  // namespace ads
