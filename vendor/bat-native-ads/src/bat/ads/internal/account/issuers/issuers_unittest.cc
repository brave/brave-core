/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/account/issuers/issuers.h"

#include <memory>

#include "bat/ads/internal/account/issuers/issuers_delegate_mock.h"
#include "bat/ads/internal/account/issuers/issuers_info.h"
#include "bat/ads/internal/account/issuers/issuers_unittest_util.h"
#include "bat/ads/internal/account/issuers/issuers_util.h"
#include "bat/ads/internal/base/net/http/http_status_code.h"
#include "bat/ads/internal/base/unittest/unittest_base.h"
#include "bat/ads/internal/base/unittest/unittest_mock_util.h"
#include "bat/ads/internal/deprecated/confirmations/confirmation_state_manager.h"

// npm run test -- brave_unit_tests --filter=BatAds*

using ::testing::_;
using ::testing::NiceMock;

namespace ads {

class BatAdsIssuersTest : public UnitTestBase {
 protected:
  BatAdsIssuersTest() = default;

  ~BatAdsIssuersTest() override = default;

  void SetUp() override {
    UnitTestBase::SetUp();

    issuers_ = std::make_unique<Issuers>();
    issuers_delegate_mock_ = std::make_unique<NiceMock<IssuersDelegateMock>>();
    issuers_->SetDelegate(issuers_delegate_mock_.get());
  }

  std::unique_ptr<Issuers> issuers_;
  std::unique_ptr<IssuersDelegateMock> issuers_delegate_mock_;
};

TEST_F(BatAdsIssuersTest, FetchIssuers) {
  // Arrange
  const URLEndpointMap& endpoints = {{// Issuers request
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

  EXPECT_CALL(*issuers_delegate_mock_, OnDidFetchIssuers(expected_issuers));
  EXPECT_CALL(*issuers_delegate_mock_, OnFailedToFetchIssuers()).Times(0);
  EXPECT_CALL(*issuers_delegate_mock_, OnWillRetryFetchingIssuers(_)).Times(0);
  EXPECT_CALL(*issuers_delegate_mock_, OnDidRetryFetchingIssuers()).Times(0);

  // Act
  issuers_->MaybeFetch();

  // Assert
}

TEST_F(BatAdsIssuersTest, FetchIssuersInvalidJsonResponse) {
  // Arrange
  const URLEndpointMap& endpoints = {{// Issuers request
                                      R"(/v1/issuers/)",
                                      {{net::HTTP_OK, "FOOBAR"}}}};
  MockUrlRequest(ads_client_mock_, endpoints);

  EXPECT_CALL(*issuers_delegate_mock_, OnDidFetchIssuers(_)).Times(0);
  EXPECT_CALL(*issuers_delegate_mock_, OnFailedToFetchIssuers()).Times(2);
  EXPECT_CALL(*issuers_delegate_mock_, OnWillRetryFetchingIssuers(_)).Times(2);
  EXPECT_CALL(*issuers_delegate_mock_, OnDidRetryFetchingIssuers());

  // Act
  issuers_->MaybeFetch();

  FastForwardClockToNextPendingTask();

  // Assert
  const IssuersInfo expected_issuers;
  EXPECT_EQ(expected_issuers, GetIssuers());
}

TEST_F(BatAdsIssuersTest, FetchIssuersNonHttpOkResponse) {
  // Arrange
  const URLEndpointMap& endpoints = {{// Issuers request
                                      R"(/v1/issuers/)",
                                      {{net::HTTP_NOT_FOUND, ""}}}};
  MockUrlRequest(ads_client_mock_, endpoints);

  EXPECT_CALL(*issuers_delegate_mock_, OnDidFetchIssuers(_)).Times(0);
  EXPECT_CALL(*issuers_delegate_mock_, OnFailedToFetchIssuers()).Times(2);
  EXPECT_CALL(*issuers_delegate_mock_, OnWillRetryFetchingIssuers(_)).Times(2);
  EXPECT_CALL(*issuers_delegate_mock_, OnDidRetryFetchingIssuers());

  // Act
  issuers_->MaybeFetch();

  FastForwardClockToNextPendingTask();

  // Assert
  const IssuersInfo expected_issuers;
  EXPECT_EQ(expected_issuers, GetIssuers());
}

}  // namespace ads
