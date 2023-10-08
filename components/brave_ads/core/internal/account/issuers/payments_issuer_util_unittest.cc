/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/account/issuers/payments_issuer_util.h"

#include "base/uuid.h"
#include "brave/components/brave_ads/core/internal/account/issuers/issuer_info.h"
#include "brave/components/brave_ads/core/internal/account/issuers/issuers_feature.h"
#include "brave/components/brave_ads/core/internal/account/issuers/issuers_info.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_base.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

class BraveAdsPaymentsIssuerUtilTest : public UnitTestBase {};

TEST_F(BraveAdsPaymentsIssuerUtilTest, IsValid) {
  // Arrange
  IssuerInfo issuer;
  issuer.type = IssuerType::kPayments;

  for (int i = 0; i < kMaximumIssuerPublicKeys.Get(); ++i) {
    issuer.public_keys.insert(
        {/*public_key=*/base::Uuid::GenerateRandomV4().AsLowercaseString(),
         /*associated_value=*/0.1});
  }

  IssuersInfo issuers;
  issuers.issuers.push_back(issuer);

  // Act & Assert
  EXPECT_TRUE(IsPaymentsIssuerValid(issuers));
}

TEST_F(BraveAdsPaymentsIssuerUtilTest, IsInvalid) {
  // Arrange
  IssuerInfo issuer;
  issuer.type = IssuerType::kPayments;

  for (int i = 0; i < kMaximumIssuerPublicKeys.Get() + 1; ++i) {
    issuer.public_keys.insert(
        {/*public_key=*/base::Uuid::GenerateRandomV4().AsLowercaseString(),
         /*associated_value=*/0.1});
  }

  IssuersInfo issuers;
  issuers.issuers.push_back(issuer);

  // Act & Assert
  EXPECT_FALSE(IsPaymentsIssuerValid(issuers));
}

}  // namespace brave_ads
