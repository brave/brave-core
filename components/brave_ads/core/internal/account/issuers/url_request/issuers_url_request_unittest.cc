/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/account/issuers/url_request/issuers_url_request.h"

#include <memory>

#include "brave/components/brave_ads/core/internal/account/issuers/issuers_info.h"
#include "brave/components/brave_ads/core/internal/account/issuers/issuers_test_util.h"
#include "brave/components/brave_ads/core/internal/account/issuers/issuers_util.h"
#include "brave/components/brave_ads/core/internal/account/issuers/url_request/issuers_url_request_builder_util.h"
#include "brave/components/brave_ads/core/internal/account/issuers/url_request/issuers_url_request_delegate_mock.h"
#include "brave/components/brave_ads/core/internal/common/test/mock_test_util.h"
#include "brave/components/brave_ads/core/internal/common/test/test_base.h"
#include "brave/components/brave_ads/core/internal/common/test/test_constants.h"
#include "net/http/http_status_code.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

class BraveAdsIssuersUrlRequestTest : public test::TestBase {
 protected:
  void SetUp() override {
    test::TestBase::SetUp();

    issuers_url_request_ = std::make_unique<IssuersUrlRequest>();
    issuers_url_request_->SetDelegate(&delegate_mock_);
  }

  std::unique_ptr<IssuersUrlRequest> issuers_url_request_;
  IssuersUrlRequestDelegateMock delegate_mock_;
};

TEST_F(BraveAdsIssuersUrlRequestTest, FetchIssuers) {
  // Arrange
  const test::URLResponseMap url_responses = {
      {BuildIssuersUrlPath(),
       {{net::HTTP_OK, test::BuildIssuersUrlResponseBody()}}}};
  test::MockUrlResponses(ads_client_mock_, url_responses);

  // Act & Assert
  EXPECT_CALL(delegate_mock_, OnFailedToFetchIssuers).Times(0);
  EXPECT_CALL(delegate_mock_, OnWillRetryFetchingIssuers).Times(0);
  EXPECT_CALL(delegate_mock_, OnDidRetryFetchingIssuers).Times(0);
  EXPECT_CALL(delegate_mock_, OnDidFetchIssuers(test::BuildIssuers()));
  issuers_url_request_->PeriodicallyFetch();
}

TEST_F(BraveAdsIssuersUrlRequestTest, DoNotFetchIssuersIfInvalidResponseBody) {
  // Arrange
  const test::URLResponseMap url_responses = {
      {BuildIssuersUrlPath(),
       {{net::HTTP_OK, /*response_body=*/test::kMalformedJson}}}};
  test::MockUrlResponses(ads_client_mock_, url_responses);

  // Act & Assert
  EXPECT_CALL(delegate_mock_, OnFailedToFetchIssuers);
  EXPECT_CALL(delegate_mock_, OnWillRetryFetchingIssuers);
  EXPECT_CALL(delegate_mock_, OnDidRetryFetchingIssuers).Times(0);
  EXPECT_CALL(delegate_mock_, OnDidFetchIssuers).Times(0);
  issuers_url_request_->PeriodicallyFetch();

  EXPECT_FALSE(GetIssuers());
}

TEST_F(BraveAdsIssuersUrlRequestTest, RetryFetchingIssuersIfNonHttpOkResponse) {
  // Arrange
  const test::URLResponseMap url_responses = {
      {BuildIssuersUrlPath(),
       {{net::HTTP_INTERNAL_SERVER_ERROR,
         /*response_body=*/net::GetHttpReasonPhrase(
             net::HTTP_INTERNAL_SERVER_ERROR)},
        {net::HTTP_OK, test::BuildIssuersUrlResponseBody()}}}};
  test::MockUrlResponses(ads_client_mock_, url_responses);

  ON_CALL(delegate_mock_, OnDidFetchIssuers)
      .WillByDefault(::testing::Invoke([](const IssuersInfo& issuers) {
        // Set issuers to prevent further retries.
        SetIssuers(issuers);
      }));

  // Act & Assert
  EXPECT_CALL(delegate_mock_, OnFailedToFetchIssuers);
  EXPECT_CALL(delegate_mock_, OnWillRetryFetchingIssuers);
  EXPECT_CALL(delegate_mock_, OnDidRetryFetchingIssuers);
  EXPECT_CALL(delegate_mock_, OnDidFetchIssuers);
  issuers_url_request_->PeriodicallyFetch();
  FastForwardClockToNextPendingTask();

  EXPECT_TRUE(HasIssuers());
}

}  // namespace brave_ads
