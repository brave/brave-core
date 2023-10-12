/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/account/issuers/public_key_util.h"

#include "brave/components/brave_ads/core/internal/account/issuers/issuer_info.h"
#include "brave/components/brave_ads/core/internal/common/challenge_bypass_ristretto/public_key.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_base.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

namespace {

constexpr char kPublicKey[] = "bCKwI6tx5LWrZKxWbW5CxaVIGe2N0qGYLfFE+38urCg=";
constexpr char kMissingPublicKey[] =
    "QnShwT9vRebch3WDu28nqlTaNCU5MaOF1n4VV4Q3K1g==";

}  // namespace

class BraveAdsPublicKeyUtilTest : public UnitTestBase {};

TEST_F(BraveAdsPublicKeyUtilTest, PublicKeyExists) {
  // Arrange
  IssuerInfo issuer;
  issuer.public_keys.insert({kPublicKey, /*associated_value=*/0.1});

  // Act & Assert
  EXPECT_TRUE(PublicKeyExists(issuer, cbr::PublicKey(kPublicKey)));
}

TEST_F(BraveAdsPublicKeyUtilTest, PublicKeyDoesNotExist) {
  // Arrange
  IssuerInfo issuer;
  issuer.public_keys.insert({kPublicKey, /*associated_value=*/0.1});

  // Act & Assert
  EXPECT_FALSE(PublicKeyExists(issuer, cbr::PublicKey(kMissingPublicKey)));
}

TEST_F(BraveAdsPublicKeyUtilTest, NoPublicKeys) {
  // Arrange
  const IssuerInfo issuer;

  // Act & Assert
  EXPECT_FALSE(PublicKeyExists(issuer, cbr::PublicKey(kPublicKey)));
}

}  // namespace brave_ads
