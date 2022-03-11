/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/account/user_data/totals_user_data.h"

#include <string>

#include "base/json/json_writer.h"
#include "base/values.h"
#include "bat/ads/internal/privacy/unblinded_payment_tokens/unblinded_payment_tokens_unittest_util.h"
#include "testing/gtest/include/gtest/gtest.h"

// npm run test -- brave_unit_tests --filter=BatAds*

namespace ads {

namespace {

std::string GetTotalsAsJson(
    const privacy::UnblindedPaymentTokenList& unblinded_payment_tokens) {
  const base::DictionaryValue user_data =
      user_data::GetTotals(unblinded_payment_tokens);

  std::string json;
  base::JSONWriter::Write(user_data, &json);

  return json;
}

}  // namespace

TEST(BatAdsTotalsUserDataTest, GetTotalsForNoUnblindedPaymentTokens) {
  // Arrange
  const privacy::UnblindedPaymentTokenList unblinded_payment_tokens;

  // Act
  const std::string json = GetTotalsAsJson(unblinded_payment_tokens);

  // Assert
  const std::string expected_json = R"({"totals":[]})";

  EXPECT_EQ(expected_json, json);
}

TEST(BatAdsTotalsUserDataTest, GetTotals) {
  // Arrange
  privacy::UnblindedPaymentTokenList unblinded_payment_tokens;

  const privacy::UnblindedPaymentTokenInfo unblinded_payment_token_1 =
      privacy::CreateUnblindedPaymentToken(ConfirmationType::kViewed,
                                           AdType::kAdNotification);
  unblinded_payment_tokens.push_back(unblinded_payment_token_1);

  const privacy::UnblindedPaymentTokenInfo unblinded_payment_token_2 =
      privacy::CreateUnblindedPaymentToken(ConfirmationType::kViewed,
                                           AdType::kAdNotification);
  unblinded_payment_tokens.push_back(unblinded_payment_token_2);

  const privacy::UnblindedPaymentTokenInfo unblinded_payment_token_3 =
      privacy::CreateUnblindedPaymentToken(ConfirmationType::kClicked,
                                           AdType::kAdNotification);
  unblinded_payment_tokens.push_back(unblinded_payment_token_3);

  const privacy::UnblindedPaymentTokenInfo unblinded_payment_token_4 =
      privacy::CreateUnblindedPaymentToken(ConfirmationType::kViewed,
                                           AdType::kInlineContentAd);
  unblinded_payment_tokens.push_back(unblinded_payment_token_4);

  // Act
  const std::string json = GetTotalsAsJson(unblinded_payment_tokens);

  // Assert
  const std::string expected_json =
      R"({"totals":[{"ad_format":"ad_notification","click":"1","view":"2"},{"ad_format":"inline_content_ad","view":"1"}]})";

  EXPECT_EQ(expected_json, json);
}

}  // namespace ads
