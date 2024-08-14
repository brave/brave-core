/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/account/user_data/fixed/summary_user_data.h"

#include "base/test/values_test_util.h"
#include "brave/components/brave_ads/core/internal/account/tokens/payment_tokens/payment_token_info.h"
#include "brave/components/brave_ads/core/internal/account/tokens/payment_tokens/payment_tokens_test_util.h"
#include "brave/components/brave_ads/core/internal/common/test/test_base.h"
#include "brave/components/brave_ads/core/internal/settings/settings_test_util.h"
#include "brave/components/brave_ads/core/public/account/confirmations/confirmation_type.h"
#include "brave/components/brave_ads/core/public/ad_units/ad_type.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

class BraveAdsSummaryUserDataTest : public test::TestBase {};

TEST_F(BraveAdsSummaryUserDataTest, BuildSummaryUserDataForRewardsUser) {
  // Arrange
  PaymentTokenList payment_tokens;

  const PaymentTokenInfo payment_token_1 = test::BuildPaymentToken(
      ConfirmationType::kViewedImpression, AdType::kNotificationAd);
  payment_tokens.push_back(payment_token_1);

  const PaymentTokenInfo payment_token_2 = test::BuildPaymentToken(
      ConfirmationType::kViewedImpression, AdType::kNotificationAd);
  payment_tokens.push_back(payment_token_2);

  const PaymentTokenInfo payment_token_3 = test::BuildPaymentToken(
      ConfirmationType::kClicked, AdType::kNotificationAd);
  payment_tokens.push_back(payment_token_3);

  const PaymentTokenInfo payment_token_4 = test::BuildPaymentToken(
      ConfirmationType::kViewedImpression, AdType::kInlineContentAd);
  payment_tokens.push_back(payment_token_4);

  // Act
  const base::Value::Dict user_data = BuildSummaryUserData(payment_tokens);

  // Assert
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
            user_data);
}

TEST_F(BraveAdsSummaryUserDataTest, BuildSummaryUserDataForNonRewardsUser) {
  // Arrange
  test::DisableBraveRewards();

  const PaymentTokenList payment_tokens = test::BuildPaymentTokens(/*count=*/3);

  // Act
  const base::Value::Dict user_data = BuildSummaryUserData(payment_tokens);

  // Assert
  EXPECT_THAT(user_data, ::testing::IsEmpty());
}

TEST_F(BraveAdsSummaryUserDataTest, BuildSummaryUserDataIfNoPaymentTokens) {
  // Act
  const base::Value::Dict user_data =
      BuildSummaryUserData(/*payment_tokens=*/{});

  // Assert
  EXPECT_EQ(base::test::ParseJsonDict(
                R"(
                    {
                      "totals": []
                    })"),
            user_data);
}

}  // namespace brave_ads
