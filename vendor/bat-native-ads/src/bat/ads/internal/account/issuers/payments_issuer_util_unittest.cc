/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/account/issuers/payments_issuer_util.h"

#include "base/guid.h"
#include "bat/ads/internal/account/issuers/issuer_constants.h"
#include "bat/ads/internal/account/issuers/issuer_info.h"
#include "bat/ads/internal/account/issuers/issuers_info.h"
#include "bat/ads/internal/common/unittest/unittest_base.h"

// npm run test -- brave_unit_tests --filter=BatAds*

namespace ads {

class BatAdsPaymentsIssuerUtilTest : public UnitTestBase {};

TEST_F(BatAdsPaymentsIssuerUtilTest, IsValid) {
  // Arrange
  IssuerInfo issuer;
  issuer.type = IssuerType::kPayments;

  for (int i = 0; i < kMaximumIssuerPublicKeys; i++) {
    issuer.public_keys.insert(
        {/*public_key*/ base::GUID::GenerateRandomV4().AsLowercaseString(),
         /*associated_value*/ 0.1});
  }

  IssuersInfo issuers;
  issuers.issuers.push_back(issuer);

  // Act

  // Assert
  EXPECT_TRUE(IsPaymentsIssuerValid(issuers));
}

TEST_F(BatAdsPaymentsIssuerUtilTest, IsInvalid) {
  // Arrange
  IssuerInfo issuer;
  issuer.type = IssuerType::kPayments;

  for (int i = 0; i < kMaximumIssuerPublicKeys + 1; i++) {
    issuer.public_keys.insert(
        {/*public_key*/ base::GUID::GenerateRandomV4().AsLowercaseString(),
         /*associated_value*/ 0.1});
  }

  IssuersInfo issuers;
  issuers.issuers.push_back(issuer);

  // Act

  // Assert
  EXPECT_FALSE(IsPaymentsIssuerValid(issuers));
}

}  // namespace ads
