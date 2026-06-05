/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/account/issuers/token_issuers/confirmation_token_issuer_util.h"

#include "base/uuid.h"
#include "brave/components/brave_ads/core/internal/account/issuers/issuers_feature.h"
#include "brave/components/brave_ads/core/internal/account/issuers/issuers_info.h"
#include "brave/components/brave_ads/core/internal/common/test/test_base.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

class BraveAdsConfirmationsIssuerUtilTest : public test::TestBase {};

TEST_F(BraveAdsConfirmationsIssuerUtilTest, IsConfirmationTokenIssuerValid) {
  // Arrange
  IssuersInfo issuers;
  for (size_t i = 0; i < kMaximumTokenIssuerPublicKeys.Get(); ++i) {
    issuers.confirmation_token_issuer.public_keys.insert(
        base::Uuid::GenerateRandomV4().AsLowercaseString());
  }

  // Act & Assert
  EXPECT_TRUE(IsConfirmationTokenIssuerValid(issuers));
}

TEST_F(BraveAdsConfirmationsIssuerUtilTest,
       IsConfirmationTokenIssuerInvalidIfExceedsMaximum) {
  // Arrange
  IssuersInfo issuers;
  for (size_t i = 0; i < kMaximumTokenIssuerPublicKeys.Get() + 1; ++i) {
    issuers.confirmation_token_issuer.public_keys.insert(
        base::Uuid::GenerateRandomV4().AsLowercaseString());
  }

  // Act & Assert
  EXPECT_FALSE(IsConfirmationTokenIssuerValid(issuers));
}

TEST_F(BraveAdsConfirmationsIssuerUtilTest,
       IsConfirmationTokenIssuerInvalidIfEmpty) {
  // Arrange
  const IssuersInfo issuers;

  // Act & Assert
  EXPECT_FALSE(IsConfirmationTokenIssuerValid(issuers));
}

}  // namespace brave_ads
