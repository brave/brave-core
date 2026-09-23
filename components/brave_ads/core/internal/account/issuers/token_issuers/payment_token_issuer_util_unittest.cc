/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/account/issuers/token_issuers/payment_token_issuer_util.h"

#include "base/uuid.h"
#include "brave/components/brave_ads/core/internal/account/issuers/issuers_feature.h"
#include "brave/components/brave_ads/core/internal/account/issuers/issuers_info.h"
#include "brave/components/brave_ads/core/internal/common/test/test_base.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

class BraveAdsPaymentsIssuerUtilTest : public test::TestBase {};

TEST_F(BraveAdsPaymentsIssuerUtilTest, IsPaymentTokenIssuerValid) {
  // Arrange
  IssuersInfo issuers;
  for (size_t i = 0; i < kMaximumTokenIssuerPublicKeys.Get(); ++i) {
    issuers.payment_token_issuer.public_keys.insert(
        {base::Uuid::GenerateRandomV4().AsLowercaseString(),
         /*bucket_value=*/0.1});
  }

  // Act & Assert
  EXPECT_TRUE(IsPaymentTokenIssuerValid(issuers));
}

TEST_F(BraveAdsPaymentsIssuerUtilTest,
       IsPaymentTokenIssuerInvalidIfExceedsMaximum) {
  // Arrange
  IssuersInfo issuers;
  for (size_t i = 0; i < kMaximumTokenIssuerPublicKeys.Get() + 1; ++i) {
    issuers.payment_token_issuer.public_keys.insert(
        {base::Uuid::GenerateRandomV4().AsLowercaseString(),
         /*bucket_value=*/0.1});
  }

  // Act & Assert
  EXPECT_FALSE(IsPaymentTokenIssuerValid(issuers));
}

TEST_F(BraveAdsPaymentsIssuerUtilTest, IsPaymentTokenIssuerInvalidIfEmpty) {
  // Arrange
  const IssuersInfo issuers;

  // Act & Assert
  EXPECT_FALSE(IsPaymentTokenIssuerValid(issuers));
}

}  // namespace brave_ads
