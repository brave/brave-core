/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/account/issuers/public_key_util.h"

#include "bat/ads/internal/account/issuers/issuer_info.h"
#include "bat/ads/internal/common/unittest/unittest_base.h"

// npm run test -- brave_unit_tests --filter=BatAds*

namespace ads {

namespace {

constexpr char kPublicKey[] = "JsvJluEN35bJBgJWTdW/8dAgPrrTM1I1pXga+o7cllo=";
constexpr char kMissingPublicKey[] =
    "crDVI1R6xHQZ4D9cQu4muVM5MaaM1QcOT4It8Y/CYlw==";

}  // namespace

class BatAdsPublicKeyUtilTest : public UnitTestBase {};

TEST_F(BatAdsPublicKeyUtilTest, PublicKeyExists) {
  // Arrange
  IssuerInfo issuer;
  issuer.public_keys.insert({kPublicKey, /*associated_value*/ 0.1});

  // Act

  // Assert
  EXPECT_TRUE(PublicKeyExists(issuer, kPublicKey));
}

TEST_F(BatAdsPublicKeyUtilTest, PublicKeyDoesNotExist) {
  // Arrange
  IssuerInfo issuer;
  issuer.public_keys.insert({kPublicKey, /*associated_value*/ 0.1});

  // Act

  // Assert
  EXPECT_FALSE(PublicKeyExists(issuer, kMissingPublicKey));
}

TEST_F(BatAdsPublicKeyUtilTest, NoPublicKeys) {
  // Arrange
  const IssuerInfo issuer;

  // Act

  // Assert
  EXPECT_FALSE(PublicKeyExists(issuer, kPublicKey));
}

}  // namespace ads
