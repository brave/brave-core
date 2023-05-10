/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/account/confirmations/redeem_unblinded_payment_tokens/redeem_unblinded_payment_tokens_user_data_builder.h"

#include "base/functional/bind.h"
#include "base/test/values_test_util.h"
#include "base/values.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_base.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_mock_util.h"
#include "brave/components/brave_ads/core/internal/privacy/tokens/unblinded_payment_tokens/unblinded_payment_tokens_unittest_util.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

class BraveAdsRedeemUnblindedPaymentTokensUserDataBuilderTest
    : public UnitTestBase {};

TEST_F(BraveAdsRedeemUnblindedPaymentTokensUserDataBuilderTest, BuildUserData) {
  // Arrange
  MockPlatformHelper(platform_helper_mock_, PlatformType::kWindows);

  const RedeemUnblindedPaymentTokensUserDataBuilder user_data_builder(
      privacy::BuildUnblindedPaymentTokens(/*count*/ 2));

  // Act
  user_data_builder.Build(base::BindOnce([](base::Value::Dict user_data) {
    EXPECT_EQ(
        base::test::ParseJsonDict(
            R"({"platform":"windows","totals":[{"ad_format":"ad_notification","view":2}]})"),
        user_data);
  }));

  // Assert
}

}  // namespace brave_ads
