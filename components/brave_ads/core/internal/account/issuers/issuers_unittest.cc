/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/account/issuers/issuers.h"

#include <memory>

#include "brave/components/brave_ads/core/internal/account/issuers/issuers_delegate_mock.h"
#include "brave/components/brave_ads/core/internal/account/issuers/issuers_info.h"
#include "brave/components/brave_ads/core/internal/account/issuers/issuers_unittest_util.h"
#include "brave/components/brave_ads/core/internal/account/issuers/issuers_url_request_builder_util.h"
#include "brave/components/brave_ads/core/internal/account/issuers/issuers_util.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_base.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_mock_util.h"
#include "net/http/http_status_code.h"
#include "third_party/abseil-cpp/absl/types/optional.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

using ::testing::_;
using ::testing::NiceMock;

class BraveAdsIssuersTest : public UnitTestBase {
 protected:
  void SetUp() override {
    UnitTestBase::SetUp();

    issuers_ = std::make_unique<Issuers>();
    issuers_->SetDelegate(&issuers_delegate_mock_);
  }

  std::unique_ptr<Issuers> issuers_;
  NiceMock<IssuersDelegateMock> issuers_delegate_mock_;
};

TEST_F(BraveAdsIssuersTest, FetchIssuers) {
  // Arrange
  const URLResponseMap url_responses = {
      {BuildIssuersUrlPath(), {{net::HTTP_OK, BuildIssuersUrlResponseBody()}}}};
  MockUrlResponses(ads_client_mock_, url_responses);

  const IssuersInfo expected_issuers =
      BuildIssuers(7'200'000,
                   {{"bCKwI6tx5LWrZKxWbW5CxaVIGe2N0qGYLfFE+38urCg=", 0.0},
                    {"crDVI1R6xHQZ4D9cQu4muVM5MaaM1QcOT4It8Y/CYlw=", 0.0}},
                   {{"JiwFR2EU/Adf1lgox+xqOVPuc6a/rxdy/LguFG5eaXg=", 0.0},
                    {"bPE1QE65mkIgytffeu7STOfly+x10BXCGuk5pVlOHQU=", 0.1}});

  EXPECT_CALL(issuers_delegate_mock_, OnDidFetchIssuers(expected_issuers));
  EXPECT_CALL(issuers_delegate_mock_, OnFailedToFetchIssuers()).Times(0);
  EXPECT_CALL(issuers_delegate_mock_, OnWillRetryFetchingIssuers(_)).Times(0);
  EXPECT_CALL(issuers_delegate_mock_, OnDidRetryFetchingIssuers()).Times(0);

  // Act
  issuers_->MaybeFetch();

  // Assert
}

TEST_F(BraveAdsIssuersTest, FetchIssuersInvalidJsonResponseBody) {
  // Arrange
  const URLResponseMap url_responses = {
      {BuildIssuersUrlPath(), {{net::HTTP_OK, /*response_body*/ "{INVALID}"}}}};
  MockUrlResponses(ads_client_mock_, url_responses);

  EXPECT_CALL(issuers_delegate_mock_, OnDidFetchIssuers(_)).Times(0);
  EXPECT_CALL(issuers_delegate_mock_, OnFailedToFetchIssuers()).Times(2);
  EXPECT_CALL(issuers_delegate_mock_, OnWillRetryFetchingIssuers(_)).Times(2);
  EXPECT_CALL(issuers_delegate_mock_, OnDidRetryFetchingIssuers());

  // Act
  issuers_->MaybeFetch();

  FastForwardClockToNextPendingTask();

  // Assert
  const absl::optional<IssuersInfo> issuers = GetIssuers();
  ASSERT_TRUE(issuers);

  const IssuersInfo expected_issuers;
  EXPECT_EQ(expected_issuers, *issuers);
}

TEST_F(BraveAdsIssuersTest, FetchIssuersNonHttpOkResponse) {
  // Arrange
  const URLResponseMap url_responses = {
      {BuildIssuersUrlPath(),
       {{net::HTTP_NOT_FOUND,
         /*response_body*/ net::GetHttpReasonPhrase(net::HTTP_NOT_FOUND)}}}};
  MockUrlResponses(ads_client_mock_, url_responses);

  EXPECT_CALL(issuers_delegate_mock_, OnDidFetchIssuers(_)).Times(0);
  EXPECT_CALL(issuers_delegate_mock_, OnFailedToFetchIssuers()).Times(2);
  EXPECT_CALL(issuers_delegate_mock_, OnWillRetryFetchingIssuers(_)).Times(2);
  EXPECT_CALL(issuers_delegate_mock_, OnDidRetryFetchingIssuers());

  // Act
  issuers_->MaybeFetch();

  FastForwardClockToNextPendingTask();

  // Assert
  const absl::optional<IssuersInfo> issuers = GetIssuers();
  ASSERT_TRUE(issuers);

  const IssuersInfo expected_issuers;
  EXPECT_EQ(expected_issuers, *issuers);
}

}  // namespace brave_ads
