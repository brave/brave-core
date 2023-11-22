/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/account/user_data/fixed/summary_user_data.h"

#include "base/test/values_test_util.h"
#include "brave/components/brave_ads/core/internal/account/tokens/payment_tokens/payment_tokens_unittest_util.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_base.h"
#include "brave/components/brave_ads/core/internal/settings/settings_unittest_util.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

class BraveAdsSummaryUserDataTest : public UnitTestBase {};

TEST_F(BraveAdsSummaryUserDataTest, BuildSummaryUserDataForRewardsUser) {
  // Arrange
  PaymentTokenList payment_tokens;

  const PaymentTokenInfo payment_token_1 = test::BuildPaymentToken(
      ConfirmationType::kViewed, AdType::kNotificationAd);
  payment_tokens.push_back(payment_token_1);

  const PaymentTokenInfo payment_token_2 = test::BuildPaymentToken(
      ConfirmationType::kViewed, AdType::kNotificationAd);
  payment_tokens.push_back(payment_token_2);

  const PaymentTokenInfo payment_token_3 = test::BuildPaymentToken(
      ConfirmationType::kClicked, AdType::kNotificationAd);
  payment_tokens.push_back(payment_token_3);

  const PaymentTokenInfo payment_token_4 = test::BuildPaymentToken(
      ConfirmationType::kViewed, AdType::kInlineContentAd);
  payment_tokens.push_back(payment_token_4);

  // Act & Assert
  EXPECT_EQ(base::test::ParseJsonDict(
                R"(
                    {
                      "totals": [
                        {
                          "ad_format": "ad_notification",
                          "click": 1,
                          "view": 2
                        },
                        {
                          "ad_format": "inline_content_ad",
                          "view": 1
                        }
                      ]
                    }
                )"),
            BuildSummaryUserData(payment_tokens));
}

TEST_F(BraveAdsSummaryUserDataTest, BuildSummaryUserDataForNonRewardsUser) {
  // Arrange
  test::DisableBraveRewards();

  const PaymentTokenList payment_tokens = test::BuildPaymentTokens(/*count=*/3);

  // Act & Assert
  EXPECT_TRUE(BuildSummaryUserData(payment_tokens).empty());
}

TEST_F(BraveAdsSummaryUserDataTest, BuildSummaryUserDataIfNoPaymentTokens) {
  // Arrange
  const PaymentTokenList payment_tokens;

  // Act & Assert
  EXPECT_EQ(base::test::ParseJsonDict(
                R"(
                    {
                      "totals": []
                    })"),
            BuildSummaryUserData(payment_tokens));
}

}  // namespace brave_ads
