/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/account/issuers/token_issuers/token_issuer_value_util.h"

#include "base/test/values_test_util.h"
#include "brave/components/brave_ads/core/internal/account/issuers/issuers_test_util.h"
#include "brave/components/brave_ads/core/internal/account/issuers/token_issuers/token_issuer_info.h"
#include "brave/components/brave_ads/core/internal/common/test/test_base.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

namespace {

constexpr char kTokenIssuersAsJson[] = R"(
  [
    {
      "name": "confirmations",
      "publicKeys": [
        {
          "publicKey": "QnShwT9vRebch3WDu28nqlTaNCU5MaOF1n4VV4Q3K1g=",
          "associatedValue": "0"
        },
        {
          "publicKey": "bCKwI6tx5LWrZKxWbW5CxaVIGe2N0qGYLfFE+38urCg=",
          "associatedValue": "0"
        }
      ]
    },
    {
      "name": "payments",
      "publicKeys": [
        {
          "publicKey": "JiwFR2EU/Adf1lgox+xqOVPuc6a/rxdy/LguFG5eaXg=",
          "associatedValue": "0"
        },
        {
          "publicKey": "bPE1QE65mkIgytffeu7STOfly+x10BXCGuk5pVlOHQU=",
          "associatedValue": "0.1"
        }
      ]
    }
  ])";

}  // namespace

class BraveAdsTokenIssuerValueUtilTest : public test::TestBase {};

TEST_F(BraveAdsTokenIssuerValueUtilTest, TokenIssuersToValue) {
  // Arrange
  const TokenIssuerList token_issuers = test::BuildTokenIssuers();

  // Act
  const base::Value::List list = TokenIssuersToValue(token_issuers);

  // Assert
  EXPECT_EQ(base::test::ParseJsonList(kTokenIssuersAsJson), list);
}

TEST_F(BraveAdsTokenIssuerValueUtilTest, EmptyTokenIssuersToValue) {
  // Act
  const base::Value::List list = TokenIssuersToValue({});

  // Assert
  EXPECT_THAT(list, ::testing::IsEmpty());
}

TEST_F(BraveAdsTokenIssuerValueUtilTest, TokenIssuersFromValue) {
  // Arrange
  const base::Value::List list = base::test::ParseJsonList(kTokenIssuersAsJson);

  // Act
  const std::optional<TokenIssuerList> token_issuers =
      TokenIssuersFromValue(list);

  // Assert
  EXPECT_EQ(test::BuildTokenIssuers(), token_issuers);
}

TEST_F(BraveAdsTokenIssuerValueUtilTest, EmptyTokenIssuersFromValue) {
  // Arrange
  const base::Value::List list = base::test::ParseJsonList("[]");

  // Act
  const std::optional<TokenIssuerList> token_issuers =
      TokenIssuersFromValue(list);

  // Assert
  EXPECT_THAT(*token_issuers, ::testing::IsEmpty());
}

}  // namespace brave_ads
