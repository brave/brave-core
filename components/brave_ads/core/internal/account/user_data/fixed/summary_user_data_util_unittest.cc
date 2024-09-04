/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/account/user_data/fixed/summary_user_data_util.h"

#include "brave/components/brave_ads/core/internal/account/tokens/payment_tokens/payment_token_info.h"
#include "brave/components/brave_ads/core/internal/account/tokens/payment_tokens/payment_tokens_test_util.h"
#include "brave/components/brave_ads/core/mojom/brave_ads.mojom.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

TEST(BraveAdsSummaryUserDataUtilTest, BuildBucketsIfNoPaymentTokens) {
  // Act
  const AdTypeBucketMap ad_type_buckets =
      BuildAdTypeBuckets(/*payment_tokens=*/{});

  // Assert
  EXPECT_THAT(ad_type_buckets, ::testing::IsEmpty());
}

TEST(BraveAdsSummaryUserDataUtilTest, BuildBuckets) {
  // Arrange
  PaymentTokenList payment_tokens;

  const PaymentTokenInfo payment_token_1 =
      test::BuildPaymentToken(mojom::ConfirmationType::kViewedImpression,
                              mojom::AdType::kNotificationAd);
  payment_tokens.push_back(payment_token_1);

  const PaymentTokenInfo payment_token_2 =
      test::BuildPaymentToken(mojom::ConfirmationType::kViewedImpression,
                              mojom::AdType::kNotificationAd);
  payment_tokens.push_back(payment_token_2);

  const PaymentTokenInfo payment_token_3 = test::BuildPaymentToken(
      mojom::ConfirmationType::kClicked, mojom::AdType::kNotificationAd);
  payment_tokens.push_back(payment_token_3);

  const PaymentTokenInfo payment_token_4 =
      test::BuildPaymentToken(mojom::ConfirmationType::kViewedImpression,
                              mojom::AdType::kInlineContentAd);
  payment_tokens.push_back(payment_token_4);

  // Act
  const AdTypeBucketMap ad_type_buckets = BuildAdTypeBuckets(payment_tokens);

  // Assert
  const AdTypeBucketMap expected_ad_type_buckets = {
      {mojom::AdType::kNotificationAd,
       {{mojom::ConfirmationType::kClicked, 1},
        {mojom::ConfirmationType::kViewedImpression, 2}}},
      {mojom::AdType::kInlineContentAd,
       {{mojom::ConfirmationType::kViewedImpression, 1}}}};
  EXPECT_EQ(expected_ad_type_buckets, ad_type_buckets);
}

}  // namespace brave_ads
