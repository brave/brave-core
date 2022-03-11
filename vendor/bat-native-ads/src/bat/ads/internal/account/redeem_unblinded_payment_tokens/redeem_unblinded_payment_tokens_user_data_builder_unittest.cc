/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/account/redeem_unblinded_payment_tokens/redeem_unblinded_payment_tokens_user_data_builder.h"

#include <string>

#include "base/json/json_writer.h"
#include "base/values.h"
#include "bat/ads/internal/privacy/unblinded_payment_tokens/unblinded_payment_tokens_unittest_util.h"
#include "bat/ads/internal/unittest_util.h"
#include "testing/gtest/include/gtest/gtest.h"

// npm run test -- brave_unit_tests --filter=BatAds*

namespace ads {

TEST(BatAdsRedeemUnblindedPaymentTokensUserDataBuilderTest, BuildUserData) {
  // Arrange
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
        R"({"odyssey":"host","totals":[{"ad_format":"ad_notification","view":"2"}]})";

    EXPECT_EQ(expected_json, json);
  });

  // Assert
}

}  // namespace ads
