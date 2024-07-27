/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/account/issuers/url_request/issuers_url_request_json_reader.h"

#include "brave/components/brave_ads/core/internal/account/issuers/issuers_info.h"
#include "brave/components/brave_ads/core/internal/account/issuers/issuers_test_util.h"
#include "brave/components/brave_ads/core/internal/common/test/test_base.h"
#include "brave/components/brave_ads/core/internal/common/test/test_constants.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

class BraveAdsIssuersUrlRequestJsonReaderTest : public test::TestBase {};

TEST(BraveAdsSubdivisionUrlRequestJsonReaderUtilTest, ReadIssuers) {
  // Act & Assert
  const IssuersInfo expected_issuers = test::BuildIssuers();

  EXPECT_EQ(expected_issuers, json::reader::ReadIssuers(R"(
    {
      "ping": 7200000,
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
    })"));
}

TEST(BraveAdsSubdivisionUrlRequestJsonReaderUtilTest,
     DoNotReadIssuersIfMalformedJson) {
  // Act & Assert
  EXPECT_FALSE(json::reader::ReadIssuers(test::kMalformedJson));
}

TEST(BraveAdsSubdivisionUrlRequestJsonReaderUtilTest,
     DoNotReadIssuersIfMissingPing) {
  // Act & Assert
  EXPECT_FALSE(json::reader::ReadIssuers(R"(
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
    })"));
}

TEST(BraveAdsSubdivisionUrlRequestJsonReaderUtilTest,
     DoNotReadIssuersIfMissingTokenIssuers) {
  // Act & Assert
  EXPECT_FALSE(json::reader::ReadIssuers(R"(
    {
      "ping": 7200000
    })"));
}

}  // namespace brave_ads
