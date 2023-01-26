/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/account/utility/redeem_unblinded_payment_tokens/redeem_unblinded_payment_tokens_user_data_builder.h"

#include "base/functional/bind.h"
#include "base/test/values_test_util.h"
#include "base/values.h"
#include "bat/ads/internal/common/unittest/unittest_base.h"
#include "bat/ads/internal/common/unittest/unittest_mock_util.h"
#include "bat/ads/internal/privacy/tokens/unblinded_payment_tokens/unblinded_payment_tokens_unittest_util.h"
#include "bat/ads/sys_info.h"

// npm run test -- brave_unit_tests --filter=BatAds*

namespace ads {

class BatAdsRedeemUnblindedPaymentTokensUserDataBuilderTest
    : public UnitTestBase {};

TEST_F(BatAdsRedeemUnblindedPaymentTokensUserDataBuilderTest, BuildUserData) {
  // Arrange
  MockPlatformHelper(platform_helper_mock_, PlatformType::kWindows);

  SysInfo().is_uncertain_future = false;

  const privacy::UnblindedPaymentTokenList unblinded_payment_tokens =
      privacy::GetUnblindedPaymentTokens(2);

  const RedeemUnblindedPaymentTokensUserDataBuilder user_data_builder(
      unblinded_payment_tokens);

  // Act
  user_data_builder.Build(base::BindOnce([](base::Value::Dict user_data) {
    const base::Value expected_user_data = base::test::ParseJson(
        R"({"odyssey":"host","platform":"windows","totals":[{"ad_format":"ad_notification","view":"2"}]})");
    ASSERT_TRUE(expected_user_data.is_dict());

    EXPECT_EQ(expected_user_data, user_data);
  }));

  // Assert
}

}  // namespace ads
