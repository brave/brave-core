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
#include "bat/ads/internal/base/http_status_code.h"
#include "bat/ads/internal/base/unittest_base.h"
#include "bat/ads/internal/base/unittest_time_util.h"
#include "bat/ads/internal/base/unittest_util.h"
#include "bat/ads/internal/deprecated/confirmations/confirmations_state.h"

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

TEST_F(BatAdsIssuersTest, FetchIssuers) {
  // Arrange
  const URLEndpoints& endpoints = {{// Issuers request
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

  EXPECT_CALL(*issuers_delegate_mock_, OnDidFetchIssuers(expected_issuers))
      .Times(1);
  EXPECT_CALL(*issuers_delegate_mock_, OnFailedToFetchIssuers()).Times(0);
  EXPECT_CALL(*issuers_delegate_mock_, OnWillRetryFetchingIssuers(_)).Times(0);
  EXPECT_CALL(*issuers_delegate_mock_, OnDidRetryFetchingIssuers()).Times(0);

  // Act
  issuers_->MaybeFetch();

  // Assert
}

TEST_F(BatAdsIssuersTest, FetchIssuersInvalidJsonResponse) {
  // Arrange
  const URLEndpoints& endpoints = {
      {// Issuers request
       R"(/v1/issuers/)",
       {{net::HTTP_OK, "FOOBAR"}, {net::HTTP_OK, "FOOBAR"}}}};

  MockUrlRequest(ads_client_mock_, endpoints);

  EXPECT_CALL(*issuers_delegate_mock_, OnDidFetchIssuers(_)).Times(0);
  EXPECT_CALL(*issuers_delegate_mock_, OnFailedToFetchIssuers()).Times(2);
  EXPECT_CALL(*issuers_delegate_mock_, OnWillRetryFetchingIssuers(_)).Times(2);
  EXPECT_CALL(*issuers_delegate_mock_, OnDidRetryFetchingIssuers()).Times(1);

  // Act
  issuers_->MaybeFetch();

  FastForwardClockBy(NextPendingTaskDelay());

  // Assert
  const IssuersInfo expected_issuers;
  EXPECT_EQ(expected_issuers, GetIssuers());
}

TEST_F(BatAdsIssuersTest, FetchIssuersNonHttpOkResponse) {
  // Arrange
  const URLEndpoints& endpoints = {
      {// Issuers request
       R"(/v1/issuers/)",
       {{net::HTTP_NOT_FOUND, ""}, {net::HTTP_NOT_FOUND, ""}}}};

  MockUrlRequest(ads_client_mock_, endpoints);

  EXPECT_CALL(*issuers_delegate_mock_, OnDidFetchIssuers(_)).Times(0);
  EXPECT_CALL(*issuers_delegate_mock_, OnFailedToFetchIssuers()).Times(2);
  EXPECT_CALL(*issuers_delegate_mock_, OnWillRetryFetchingIssuers(_)).Times(2);
  EXPECT_CALL(*issuers_delegate_mock_, OnDidRetryFetchingIssuers()).Times(1);

  // Act
  issuers_->MaybeFetch();

  FastForwardClockBy(NextPendingTaskDelay());

  // Assert
  const IssuersInfo expected_issuers;
  EXPECT_EQ(expected_issuers, GetIssuers());
}

TEST_F(BatAdsIssuersTest, FetchIssuersHttpUpgradeRequiredResponse) {
  // Arrange
  const URLEndpoints& endpoints = {{// Issuers request
                                    R"(/v1/issuers/)",
                                    {{net::HTTP_UPGRADE_REQUIRED, ""}}}};

  MockUrlRequest(ads_client_mock_, endpoints);

  EXPECT_CALL(*issuers_delegate_mock_, OnDidFetchIssuers(_)).Times(0);
  EXPECT_CALL(*issuers_delegate_mock_, OnFailedToFetchIssuers()).Times(1);
  EXPECT_CALL(*issuers_delegate_mock_, OnWillRetryFetchingIssuers(_)).Times(0);
  EXPECT_CALL(*issuers_delegate_mock_, OnDidRetryFetchingIssuers()).Times(0);

  // Act
  issuers_->MaybeFetch();

  // Assert
  const IssuersInfo expected_issuers;
  EXPECT_EQ(expected_issuers, GetIssuers());
}

}  // namespace ads
