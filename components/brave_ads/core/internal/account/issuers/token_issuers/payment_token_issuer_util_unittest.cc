/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/account/issuers/token_issuers/payment_token_issuer_util.h"

#include "base/uuid.h"
#include "brave/components/brave_ads/core/internal/account/issuers/issuers_feature.h"
#include "brave/components/brave_ads/core/internal/account/issuers/issuers_info.h"
#include "brave/components/brave_ads/core/internal/account/issuers/token_issuers/token_issuer_info.h"
#include "brave/components/brave_ads/core/internal/common/test/test_base.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

class BraveAdsPaymentsIssuerUtilTest : public test::TestBase {};

TEST_F(BraveAdsPaymentsIssuerUtilTest, IsPaymentTokenIssuerValid) {
  // Arrange
  TokenIssuerInfo token_issuer;
  token_issuer.type = TokenIssuerType::kPayments;

  for (int i = 0; i < kMaximumTokenIssuerPublicKeys.Get(); ++i) {
    token_issuer.public_keys.insert(
        {/*public_key=*/base::Uuid::GenerateRandomV4().AsLowercaseString(),
         /*associated_value=*/0.1});
  }

  IssuersInfo issuers;
  issuers.token_issuers.push_back(token_issuer);

  // Act & Assert
  EXPECT_TRUE(IsPaymentTokenIssuerValid(issuers));
}

TEST_F(BraveAdsPaymentsIssuerUtilTest, IsPaymentTokenIssuerInvalid) {
  // Arrange
  TokenIssuerInfo token_issuer;
  token_issuer.type = TokenIssuerType::kPayments;

  for (int i = 0; i < kMaximumTokenIssuerPublicKeys.Get() + 1; ++i) {
    token_issuer.public_keys.insert(
        {/*public_key=*/base::Uuid::GenerateRandomV4().AsLowercaseString(),
         /*associated_value=*/0.1});
  }

  IssuersInfo issuers;
  issuers.token_issuers.push_back(token_issuer);

  // Act & Assert
  EXPECT_FALSE(IsPaymentTokenIssuerValid(issuers));
}

}  // namespace brave_ads
