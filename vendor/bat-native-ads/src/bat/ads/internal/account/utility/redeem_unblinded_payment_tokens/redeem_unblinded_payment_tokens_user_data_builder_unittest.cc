/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/account/utility/redeem_unblinded_payment_tokens/redeem_unblinded_payment_tokens_user_data_builder.h"

#include <string>

#include "base/json/json_writer.h"
#include "base/values.h"
#include "bat/ads/internal/base/unittest_base.h"
#include "bat/ads/internal/base/unittest_util.h"
#include "bat/ads/internal/privacy/tokens/unblinded_payment_tokens/unblinded_payment_tokens_unittest_util.h"

// npm run test -- brave_unit_tests --filter=BatAds*

namespace ads {

class BatAdsRedeemUnblindedPaymentTokensUserDataBuilderTest
    : public UnitTestBase {
 protected:
  BatAdsRedeemUnblindedPaymentTokensUserDataBuilderTest() = default;

  ~BatAdsRedeemUnblindedPaymentTokensUserDataBuilderTest() override = default;
};

TEST_F(BatAdsRedeemUnblindedPaymentTokensUserDataBuilderTest, BuildUserData) {
  // Arrange
  MockPlatformHelper(platform_helper_mock_, PlatformType::kWindows);

  mojom::SysInfo sys_info;
  sys_info.is_uncertain_future = false;
  SetSysInfo(sys_info);

  const privacy::UnblindedPaymentTokenList unblinded_payment_tokens =
      privacy::GetUnblindedPaymentTokens(2);

  RedeemUnblindedPaymentTokensUserDataBuilder user_data_builder(
      unblinded_payment_tokens);

  // Act
  user_data_builder.Build([](const base::Value& user_data) {
    std::string json;
    base::JSONWriter::Write(user_data, &json);

    const std::string expected_json =
        R"({"odyssey":"host","platform":"windows","totals":[{"ad_format":"ad_notification","view":"2"}]})";

    EXPECT_EQ(expected_json, json);
  });

  // Assert
}

}  // namespace ads
