/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/account/issuers/issuers.h"

#include <memory>

#include "bat/ads/internal/account/confirmations/confirmations_state.h"
#include "bat/ads/internal/account/issuers/issuers_delegate_mock.h"
#include "bat/ads/internal/account/issuers/issuers_info.h"
#include "bat/ads/internal/account/issuers/issuers_unittest_util.h"
#include "bat/ads/internal/account/issuers/issuers_util.h"
#include "bat/ads/internal/unittest_base.h"
#include "bat/ads/internal/unittest_util.h"
#include "net/http/http_status_code.h"

// npm run test -- brave_unit_tests --filter=BatAds*

using ::testing::_;
using ::testing::NiceMock;

namespace ads {

class BatAdsIssuersTest : public UnitTestBase {
 protected:
  BatAdsIssuersTest()
      : issuers_(std::make_unique<Issuers>()),
        issuers_delegate_mock_(
            std::make_unique<NiceMock<IssuersDelegateMock>>()) {
    issuers_->set_delegate(issuers_delegate_mock_.get());
  }

  ~BatAdsIssuersTest() override = default;

  std::unique_ptr<Issuers> issuers_;
  std::unique_ptr<IssuersDelegateMock> issuers_delegate_mock_;
};

TEST_F(BatAdsIssuersTest, GetIssuers) {
  // Arrange
  const URLEndpoints& endpoints = {{// Get issuers request
                                    R"(/v1/issuers/)",
                                    {{net::HTTP_OK, R"(
        {
          "ping": 7200000,
          "issuers": [
            {
              "name": "confirmations",
              "publicKeys": [
                {
                  "publicKey": "JsvJluEN35bJBgJWTdW/8dAgPrrTM1I1pXga+o7cllo=",
                  "associatedValue": ""
                },
                {
                  "publicKey": "crDVI1R6xHQZ4D9cQu4muVM5MaaM1QcOT4It8Y/CYlw=",
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
        }
        )"}}}};

  MockUrlRequest(ads_client_mock_, endpoints);

  const IssuersInfo& expected_issuers =
      BuildIssuers(7200000,
                   {{"JsvJluEN35bJBgJWTdW/8dAgPrrTM1I1pXga+o7cllo=", 0.0},
                    {"crDVI1R6xHQZ4D9cQu4muVM5MaaM1QcOT4It8Y/CYlw=", 0.0}},
                   {{"JiwFR2EU/Adf1lgox+xqOVPuc6a/rxdy/LguFG5eaXg=", 0.0},
                    {"bPE1QE65mkIgytffeu7STOfly+x10BXCGuk5pVlOHQU=", 0.1}});

  EXPECT_CALL(*issuers_delegate_mock_, OnDidGetIssuers(expected_issuers))
      .Times(1);

  EXPECT_CALL(*issuers_delegate_mock_, OnFailedToGetIssuers()).Times(0);

  // Act
  issuers_->MaybeFetch();

  // Assert
}

TEST_F(BatAdsIssuersTest, GetIssuersInvalidJsonResponse) {
  // Arrange
  const URLEndpoints& endpoints = {{// Get issuers request
                                    R"(/v1/issuers/)",
                                    {{net::HTTP_OK, "FOOBAR"}}}};

  MockUrlRequest(ads_client_mock_, endpoints);

  EXPECT_CALL(*issuers_delegate_mock_, OnDidGetIssuers(_)).Times(0);

  EXPECT_CALL(*issuers_delegate_mock_, OnFailedToGetIssuers()).Times(1);

  // Act
  issuers_->MaybeFetch();

  // Assert
  const IssuersInfo expected_issuers;

  const IssuersInfo& issuers = GetIssuers();

  EXPECT_EQ(expected_issuers, issuers);
}

TEST_F(BatAdsIssuersTest, GetIssuersNonHttpOkResponse) {
  // Arrange
  const URLEndpoints& endpoints = {{// Get issuers request
                                    R"(/v1/issuers/)",
                                    {{net::HTTP_NOT_FOUND, ""}}}};

  MockUrlRequest(ads_client_mock_, endpoints);

  EXPECT_CALL(*issuers_delegate_mock_, OnDidGetIssuers(_)).Times(0);

  EXPECT_CALL(*issuers_delegate_mock_, OnFailedToGetIssuers()).Times(1);

  // Act
  issuers_->MaybeFetch();

  // Assert
  const IssuersInfo expected_issuers;

  const IssuersInfo& issuers = GetIssuers();

  EXPECT_EQ(expected_issuers, issuers);
}

}  // namespace ads
