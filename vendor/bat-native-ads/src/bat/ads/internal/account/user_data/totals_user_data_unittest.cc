/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/account/user_data/totals_user_data.h"

#include "base/test/values_test_util.h"
#include "bat/ads/internal/privacy/tokens/unblinded_payment_tokens/unblinded_payment_tokens_unittest_util.h"
#include "testing/gtest/include/gtest/gtest.h"  // IWYU pragma: keep

// npm run test -- brave_unit_tests --filter=BatAds*

namespace ads::user_data {

TEST(BatAdsTotalsUserDataTest, GetTotalsForNoUnblindedPaymentTokens) {
  // Arrange
  const privacy::UnblindedPaymentTokenList unblinded_payment_tokens;

  // Act
  const base::Value::Dict user_data = GetTotals(unblinded_payment_tokens);

  // Assert
  const base::Value expected_user_data =
      base::test::ParseJson(R"({"totals":[]})");
  ASSERT_TRUE(expected_user_data.is_dict());

  EXPECT_EQ(expected_user_data, user_data);
}

TEST(BatAdsTotalsUserDataTest, GetTotals) {
  // Arrange
  privacy::UnblindedPaymentTokenList unblinded_payment_tokens;

  const privacy::UnblindedPaymentTokenInfo unblinded_payment_token_1 =
      privacy::CreateUnblindedPaymentToken(ConfirmationType::kViewed,
                                           AdType::kNotificationAd);
  unblinded_payment_tokens.push_back(unblinded_payment_token_1);

  const privacy::UnblindedPaymentTokenInfo unblinded_payment_token_2 =
      privacy::CreateUnblindedPaymentToken(ConfirmationType::kViewed,
                                           AdType::kNotificationAd);
  unblinded_payment_tokens.push_back(unblinded_payment_token_2);

  const privacy::UnblindedPaymentTokenInfo unblinded_payment_token_3 =
      privacy::CreateUnblindedPaymentToken(ConfirmationType::kClicked,
                                           AdType::kNotificationAd);
  unblinded_payment_tokens.push_back(unblinded_payment_token_3);

  const privacy::UnblindedPaymentTokenInfo unblinded_payment_token_4 =
      privacy::CreateUnblindedPaymentToken(ConfirmationType::kViewed,
                                           AdType::kInlineContentAd);
  unblinded_payment_tokens.push_back(unblinded_payment_token_4);

  // Act
  const base::Value::Dict user_data = GetTotals(unblinded_payment_tokens);

  // Assert
  const base::Value expected_user_data = base::test::ParseJson(
      R"({"totals":[{"ad_format":"ad_notification","click":"1","view":"2"},{"ad_format":"inline_content_ad","view":"1"}]})");
  ASSERT_TRUE(expected_user_data.is_dict());

  EXPECT_EQ(expected_user_data, user_data);
}

}  // namespace ads::user_data
