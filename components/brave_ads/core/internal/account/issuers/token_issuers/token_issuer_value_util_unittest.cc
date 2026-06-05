/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/account/issuers/token_issuers/token_issuer_value_util.h"

#include <string_view>

#include "base/test/values_test_util.h"
#include "brave/components/brave_ads/core/internal/account/issuers/test/issuers_test_util.h"
#include "brave/components/brave_ads/core/internal/common/test/test_base.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

namespace {

constexpr std::string_view kTokenIssuersAsJson = R"JSON(
  [
    {
      "name": "confirmations",
      "publicKeys": [
        {
          "publicKey": "OqhZpUC8B15u+Gc11rQYRl8O3zOSAUIEC2JuDHI32TM=",
          "associatedValue": ""
        },
        {
          "publicKey": "QnShwT9vRebch3WDu28nqlTaNCU5MaOF1n4VV4Q3K1g=",
          "associatedValue": ""
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
          "publicKey": "OqhZpUC8B15u+Gc11rQYRl8O3zOSAUIEC2JuDHI32TM=",
          "associatedValue": "0.1"
        }
      ]
    }
  ])JSON";

}  // namespace

class BraveAdsTokenIssuerValueUtilTest : public test::TestBase {};

TEST_F(BraveAdsTokenIssuerValueUtilTest, TokenIssuersToList) {
  // Arrange
  const IssuersInfo issuers = test::BuildIssuers();

  // Act & Assert
  EXPECT_EQ(base::test::ParseJsonList(kTokenIssuersAsJson),
            TokenIssuersToList(issuers.confirmation_token_issuer,
                               issuers.payment_token_issuer));
}

TEST_F(BraveAdsTokenIssuerValueUtilTest, EmptyTokenIssuersToList) {
  EXPECT_THAT(TokenIssuersToList(ConfirmationTokenIssuerInfo{},
                                 PaymentTokenIssuerInfo{}),
              ::testing::SizeIs(2));
}

TEST_F(BraveAdsTokenIssuerValueUtilTest, MaybeBuildTokenIssuersFromList) {
  // Arrange
  const base::ListValue list = base::test::ParseJsonList(kTokenIssuersAsJson);

  // Act
  auto token_issuers = MaybeBuildTokenIssuersFromList(list);
  ASSERT_TRUE(token_issuers);

  // Assert
  const IssuersInfo issuers = test::BuildIssuers();
  EXPECT_EQ(issuers.confirmation_token_issuer, token_issuers->confirmation);
  EXPECT_EQ(issuers.payment_token_issuer, token_issuers->payment);
}

TEST_F(BraveAdsTokenIssuerValueUtilTest, DoNotBuildTokenIssuersFromEmptyList) {
  EXPECT_FALSE(MaybeBuildTokenIssuersFromList({}));
}

}  // namespace brave_ads
