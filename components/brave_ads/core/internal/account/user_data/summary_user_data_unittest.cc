/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/account/user_data/summary_user_data.h"

#include "base/test/values_test_util.h"
#include "brave/components/brave_ads/core/internal/privacy/tokens/unblinded_payment_tokens/unblinded_payment_tokens_unittest_util.h"
#include "testing/gtest/include/gtest/gtest.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

TEST(BraveAdsSummaryUserDataTest,
     BuildSummaryUserDataIfNoUnblindedPaymentTokens) {
  // Arrange
  const privacy::UnblindedPaymentTokenList unblinded_payment_tokens;

  // Act

  // Assert
  EXPECT_EQ(base::test::ParseJsonDict(R"({"totals":[]})"),
            BuildSummaryUserData(unblinded_payment_tokens));
}

TEST(BraveAdsSummaryUserDataTest, BuildSummaryUserData) {
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

  // Assert
  EXPECT_EQ(
      base::test::ParseJsonDict(
          R"({"totals":[{"ad_format":"ad_notification","click":1,"view":2},{"ad_format":"inline_content_ad","view":1}]})"),
      BuildSummaryUserData(unblinded_payment_tokens));
}

}  // namespace brave_ads
