/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/account/issuers/token_issuers/token_issuer_util.h"

#include "brave/components/brave_ads/core/internal/account/issuers/issuers_info.h"
#include "brave/components/brave_ads/core/internal/account/issuers/issuers_test_util.h"
#include "brave/components/brave_ads/core/internal/account/issuers/issuers_util.h"
#include "brave/components/brave_ads/core/internal/common/challenge_bypass_ristretto/public_key.h"
#include "brave/components/brave_ads/core/internal/common/test/test_base.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

class BraveAdsTokenIssuerUtilTest : public test::TestBase {};

TEST_F(BraveAdsTokenIssuerUtilTest, TokenIssuerExistsForConfirmationType) {
  // Arrange
  test::BuildAndSetIssuers();

  // Act & Assert
  EXPECT_TRUE(TokenIssuerExistsForType(TokenIssuerType::kConfirmations));
}

TEST_F(BraveAdsTokenIssuerUtilTest,
       TokenIssuerDoesNotExistForConfirmationType) {
  // Arrange
  const IssuersInfo issuers = test::BuildIssuers(
      /*ping=*/7'200'000, /*confirmation_token_issuer_public_keys=*/{},
      /*payment_token_issuer_public_keys=*/
      {{"JiwFR2EU/Adf1lgox+xqOVPuc6a/rxdy/LguFG5eaXg=", 0.0},
       {"bPE1QE65mkIgytffeu7STOfly+x10BXCGuk5pVlOHQU=", 0.1}});

  SetIssuers(issuers);

  // Act & Assert
  EXPECT_FALSE(TokenIssuerExistsForType(TokenIssuerType::kConfirmations));
}

TEST_F(BraveAdsTokenIssuerUtilTest, TokenIssuerExistsForPaymentsType) {
  // Arrange
  test::BuildAndSetIssuers();

  // Act & Assert
  EXPECT_TRUE(TokenIssuerExistsForType(TokenIssuerType::kPayments));
}

TEST_F(BraveAdsTokenIssuerUtilTest, TokenIssuerDoesNotExistForPaymentsType) {
  // Arrange
  const IssuersInfo issuers = test::BuildIssuers(
      /*ping=*/7'200'000,
      /*confirmation_token_issuer_public_keys=*/
      {{"bCKwI6tx5LWrZKxWbW5CxaVIGe2N0qGYLfFE+38urCg=", 0.0},
       {"cKo0rk1iS8Obgyni0X3RRoydDIGHsivTkfX/TM1Xl24=", 0.0}},
      /*payment_token_issuer_public_keys=*/{});

  SetIssuers(issuers);

  // Act & Assert
  EXPECT_FALSE(TokenIssuerExistsForType(TokenIssuerType::kPayments));
}

TEST_F(BraveAdsTokenIssuerUtilTest,
       TokenIssuerPublicKeyExistsForConfirmationsType) {
  // Arrange
  test::BuildAndSetIssuers();

  // Act & Assert
  EXPECT_TRUE(TokenIssuerPublicKeyExistsForType(
      TokenIssuerType::kConfirmations,
      cbr::PublicKey("QnShwT9vRebch3WDu28nqlTaNCU5MaOF1n4VV4Q3K1g=")));
}

TEST_F(BraveAdsTokenIssuerUtilTest,
       TokenIssuerPublicKeyDoesNotExistForConfirmationsType) {
  // Arrange
  test::BuildAndSetIssuers();

  // Act & Assert
  EXPECT_FALSE(TokenIssuerPublicKeyExistsForType(
      TokenIssuerType::kConfirmations,
      cbr::PublicKey("Nj2NZ6nJUsK5MJ9ga9tfyctxzpT+GlvENF2TRHU4kBg=")));
}

TEST_F(BraveAdsTokenIssuerUtilTest, TokenIssuerPublicKeyExistsForPaymentsType) {
  // Arrange
  test::BuildAndSetIssuers();

  // Act & Assert
  EXPECT_TRUE(TokenIssuerPublicKeyExistsForType(
      TokenIssuerType::kPayments,
      cbr::PublicKey("bPE1QE65mkIgytffeu7STOfly+x10BXCGuk5pVlOHQU=")));
}

TEST_F(BraveAdsTokenIssuerUtilTest,
       TokenIssuerPublicKeyDoesNotExistForPaymentsType) {
  // Arrange
  test::BuildAndSetIssuers();

  // Act & Assert
  EXPECT_FALSE(TokenIssuerPublicKeyExistsForType(
      TokenIssuerType::kPayments,
      cbr::PublicKey("zNWjpwIbghgXvTol3XPLKV3NJoEFtvUoPMiKstiWm3A=")));
}

TEST_F(BraveAdsTokenIssuerUtilTest, GetTokenIssuerForType) {
  // Arrange
  const IssuersInfo issuers = test::BuildIssuers(
      /*ping=*/7'200'000,
      /*confirmation_token_issuer_public_keys=*/
      {{"bCKwI6tx5LWrZKxWbW5CxaVIGe2N0qGYLfFE+38urCg=", 0.0},
       {"QnShwT9vRebch3WDu28nqlTaNCU5MaOF1n4VV4Q3K1g=", 0.0}},
      /*payment_token_issuer_public_keys=*/
      {{"JiwFR2EU/Adf1lgox+xqOVPuc6a/rxdy/LguFG5eaXg=", 0.0},
       {"bPE1QE65mkIgytffeu7STOfly+x10BXCGuk5pVlOHQU=", 0.1}});

  // Act
  const std::optional<TokenIssuerInfo> token_issuer =
      GetTokenIssuerForType(issuers, TokenIssuerType::kPayments);
  ASSERT_TRUE(token_issuer);

  // Assert
  EXPECT_THAT(*token_issuer,
              ::testing::FieldsAre(
                  TokenIssuerType::kPayments,
                  TokenIssuerPublicKeyMap{
                      {"JiwFR2EU/Adf1lgox+xqOVPuc6a/rxdy/LguFG5eaXg=", 0.0},
                      {"bPE1QE65mkIgytffeu7STOfly+x10BXCGuk5pVlOHQU=", 0.1}}));
}

TEST_F(BraveAdsTokenIssuerUtilTest, DoNotGetTokenIssuerForMissingType) {
  // Arrange
  const IssuersInfo issuers = test::BuildIssuers(
      /*ping=*/7'200'000,
      /*confirmation_token_issuer_public_keys=*/
      {{"bCKwI6tx5LWrZKxWbW5CxaVIGe2N0qGYLfFE+38urCg=", 0.0},
       {"QnShwT9vRebch3WDu28nqlTaNCU5MaOF1n4VV4Q3K1g=", 0.0}},
      /*payment_token_issuer_public_keys=*/{});

  // Act & Assert
  EXPECT_FALSE(GetTokenIssuerForType(issuers, TokenIssuerType::kPayments));
}

}  // namespace brave_ads
