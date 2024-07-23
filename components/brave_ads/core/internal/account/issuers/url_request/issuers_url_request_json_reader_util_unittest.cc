/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/account/issuers/url_request/issuers_url_request_json_reader_util.h"

#include "base/test/values_test_util.h"
#include "brave/components/brave_ads/core/internal/account/issuers/issuers_test_util.h"
#include "brave/components/brave_ads/core/internal/common/test/test_base.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {
class BraveAdsIssuersUrlRequestJsonReaderUtilTest : public test::TestBase {};

TEST_F(BraveAdsIssuersUrlRequestJsonReaderUtilTest, ParsePing) {
  // Arrange
  const base::Value::Dict dict = base::test::ParseJsonDict(R"(
    {
      "ping": 7200000
    })");

  // Act & Assert
  EXPECT_EQ(7'200'000, json::reader::ParsePing(dict));
}

TEST_F(BraveAdsIssuersUrlRequestJsonReaderUtilTest, DoNotParseMissingPing) {
  // Arrange
  const base::Value::Dict dict = base::test::ParseJsonDict("{}");

  // Act & Assert
  EXPECT_FALSE(json::reader::ParsePing(dict));
}

TEST_F(BraveAdsIssuersUrlRequestJsonReaderUtilTest, ParseTokenIssuers) {
  // Arrange
  const base::Value::Dict dict = base::test::ParseJsonDict(R"(
    {
      "issuers": [
        {
          "name": "confirmations",
          "publicKeys": [
            {
              "publicKey": "bCKwI6tx5LWrZKxWbW5CxaVIGe2N0qGYLfFE+38urCg=",
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
              "associatedValue": "0.0"
            },
            {
              "publicKey": "bPE1QE65mkIgytffeu7STOfly+x10BXCGuk5pVlOHQU=",
              "associatedValue": "0.1"
            }
          ]
        }
      ]
    })");

  // Act & Assert
  EXPECT_EQ(test::BuildTokenIssuers(), json::reader::ParseTokenIssuers(dict));
}

TEST_F(BraveAdsIssuersUrlRequestJsonReaderUtilTest,
       DoNotParseMissingTokenIssuers) {
  // Arrange
  const base::Value::Dict dict = base::test::ParseJsonDict("{}");

  // Act & Assert
  EXPECT_FALSE(json::reader::ParseTokenIssuers(dict));
}

}  // namespace brave_ads
