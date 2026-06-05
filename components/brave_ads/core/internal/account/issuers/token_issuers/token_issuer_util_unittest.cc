/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/account/issuers/token_issuers/token_issuer_util.h"

#include "brave/components/brave_ads/core/internal/account/issuers/issuers_util.h"
#include "brave/components/brave_ads/core/internal/account/issuers/test/issuers_test_util.h"
#include "brave/components/brave_ads/core/internal/common/challenge_bypass_ristretto/public_key.h"
#include "brave/components/brave_ads/core/internal/common/test/test_base.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

class BraveAdsTokenIssuerUtilTest : public test::TestBase {};

TEST_F(BraveAdsTokenIssuerUtilTest, ConfirmationTokenIssuerExists) {
  // Arrange
  test::BuildAndSetIssuers();

  // Act & Assert
  EXPECT_TRUE(ConfirmationTokenIssuerExists());
}

TEST_F(BraveAdsTokenIssuerUtilTest, ConfirmationTokenIssuerDoesNotExist) {
  // Arrange
  SetIssuers(test::BuildIssuers(
      /*ping=*/7'200'000, /*confirmation_public_keys=*/{},
      /*payment_public_keys=*/
      {{"JiwFR2EU/Adf1lgox+xqOVPuc6a/rxdy/LguFG5eaXg=", 0.0},
       {"OqhZpUC8B15u+Gc11rQYRl8O3zOSAUIEC2JuDHI32TM=", 0.1}}));

  // Act & Assert
  EXPECT_FALSE(ConfirmationTokenIssuerExists());
}

TEST_F(BraveAdsTokenIssuerUtilTest, PaymentTokenIssuerExists) {
  // Arrange
  test::BuildAndSetIssuers();

  // Act & Assert
  EXPECT_TRUE(PaymentTokenIssuerExists());
}

TEST_F(BraveAdsTokenIssuerUtilTest, PaymentTokenIssuerDoesNotExist) {
  // Arrange
  SetIssuers(test::BuildIssuers(
      /*ping=*/7'200'000,
      /*confirmation_public_keys=*/
      {"OqhZpUC8B15u+Gc11rQYRl8O3zOSAUIEC2JuDHI32TM=",
       "cKo0rk1iS8Obgyni0X3RRoydDIGHsivTkfX/TM1Xl24="},
      /*payment_public_keys=*/{}));

  // Act & Assert
  EXPECT_FALSE(PaymentTokenIssuerExists());
}

TEST_F(BraveAdsTokenIssuerUtilTest, ConfirmationTokenIssuerPublicKeyExists) {
  // Arrange
  test::BuildAndSetIssuers();

  // Act & Assert
  EXPECT_TRUE(ConfirmationTokenIssuerPublicKeyExists(
      cbr::PublicKey("QnShwT9vRebch3WDu28nqlTaNCU5MaOF1n4VV4Q3K1g=")));
}

TEST_F(BraveAdsTokenIssuerUtilTest,
       ConfirmationTokenIssuerPublicKeyDoesNotExist) {
  // Arrange
  test::BuildAndSetIssuers();

  // Act & Assert
  EXPECT_FALSE(ConfirmationTokenIssuerPublicKeyExists(
      cbr::PublicKey("Nj2NZ6nJUsK5MJ9ga9tfyctxzpT+GlvENF2TRHU4kBg=")));
}

TEST_F(BraveAdsTokenIssuerUtilTest, PaymentTokenIssuerPublicKeyExists) {
  // Arrange
  test::BuildAndSetIssuers();

  // Act & Assert
  EXPECT_TRUE(PaymentTokenIssuerPublicKeyExists(
      cbr::PublicKey("OqhZpUC8B15u+Gc11rQYRl8O3zOSAUIEC2JuDHI32TM=")));
}

TEST_F(BraveAdsTokenIssuerUtilTest, PaymentTokenIssuerPublicKeyDoesNotExist) {
  // Arrange
  test::BuildAndSetIssuers();

  // Act & Assert
  EXPECT_FALSE(PaymentTokenIssuerPublicKeyExists(
      cbr::PublicKey("zNWjpwIbghgXvTol3XPLKV3NJoEFtvUoPMiKstiWm3A=")));
}

}  // namespace brave_ads
