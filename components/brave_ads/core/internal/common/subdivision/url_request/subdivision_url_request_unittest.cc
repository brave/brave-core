/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/common/subdivision/url_request/subdivision_url_request.h"

#include <memory>
#include <string>

#include "brave/components/brave_ads/core/internal/common/subdivision/url_request/subdivision_url_request_builder_util.h"
#include "brave/components/brave_ads/core/internal/common/subdivision/url_request/test/subdivision_url_request_delegate_mock.h"
#include "brave/components/brave_ads/core/internal/common/subdivision/url_request/test/subdivision_url_request_test_util.h"
#include "brave/components/brave_ads/core/internal/common/test/mock_test_util.h"
#include "brave/components/brave_ads/core/internal/common/test/test_base.h"
#include "net/http/http_status_code.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

class BraveAdsSubdivisionUrlRequestTest : public test::TestBase {
 protected:
  void SetUp() override {
    test::TestBase::SetUp();

    subdivision_url_request_ = std::make_unique<SubdivisionUrlRequest>();
    subdivision_url_request_->SetDelegate(&delegate_mock_);
  }

  std::unique_ptr<SubdivisionUrlRequest> subdivision_url_request_;
  ::testing::NiceMock<SubdivisionUrlRequestDelegateMock> delegate_mock_;
};

TEST_F(BraveAdsSubdivisionUrlRequestTest,
       SchedulesPeriodicFetchAfterHttpOkResponseStatusCode) {
  // Arrange
  const test::URLResponseMap url_responses = {
      {BuildSubdivisionUrlPath(),
       {{net::HTTP_OK,
         test::BuildSubdivisionUrlResponseBody(/*country_code=*/"US",
                                               /*subdivision_code=*/"CA")}}}};
  test::MockUrlResponses(ads_client_mock_, url_responses);

  // Act
  subdivision_url_request_->PeriodicallyFetch();

  // Assert
  EXPECT_TRUE(HasPendingTasks());
}

TEST_F(BraveAdsSubdivisionUrlRequestTest,
       SchedulesRetryAfterHttpInternalServerErrorResponseStatusCode) {
  // Arrange
  const test::URLResponseMap url_responses = {
      {BuildSubdivisionUrlPath(),
       {{net::HTTP_INTERNAL_SERVER_ERROR,
         std::string(
             net::GetHttpReasonPhrase(net::HTTP_INTERNAL_SERVER_ERROR))}}}};
  test::MockUrlResponses(ads_client_mock_, url_responses);

  // Act
  subdivision_url_request_->PeriodicallyFetch();

  // Assert
  EXPECT_TRUE(HasPendingTasks());
}

TEST_F(BraveAdsSubdivisionUrlRequestTest,
       SchedulesRetryAfterHttpUnauthorizedResponseStatusCode) {
  // Arrange
  const test::URLResponseMap url_responses = {
      {BuildSubdivisionUrlPath(),
       {{net::HTTP_UNAUTHORIZED,
         std::string(net::GetHttpReasonPhrase(net::HTTP_UNAUTHORIZED))}}}};
  test::MockUrlResponses(ads_client_mock_, url_responses);

  // Act
  subdivision_url_request_->PeriodicallyFetch();

  // Assert
  EXPECT_TRUE(HasPendingTasks());
}

TEST_F(BraveAdsSubdivisionUrlRequestTest,
       DoNotRetryAfterHttpForbiddenResponseStatusCode) {
  // Arrange
  const test::URLResponseMap url_responses = {
      {BuildSubdivisionUrlPath(),
       {{net::HTTP_FORBIDDEN,
         std::string(net::GetHttpReasonPhrase(net::HTTP_FORBIDDEN))}}}};
  test::MockUrlResponses(ads_client_mock_, url_responses);

  // Act
  subdivision_url_request_->PeriodicallyFetch();

  // Assert
  EXPECT_FALSE(HasPendingTasks());
}

TEST_F(BraveAdsSubdivisionUrlRequestTest,
       SchedulesRetryAfterHttpTooManyRequestsResponseStatusCode) {
  // Arrange
  const test::URLResponseMap url_responses = {
      {BuildSubdivisionUrlPath(),
       {{net::HTTP_TOO_MANY_REQUESTS,
         std::string(net::GetHttpReasonPhrase(net::HTTP_TOO_MANY_REQUESTS))}}}};
  test::MockUrlResponses(ads_client_mock_, url_responses);

  // Act
  subdivision_url_request_->PeriodicallyFetch();

  // Assert
  EXPECT_TRUE(HasPendingTasks());
}

TEST_F(
    BraveAdsSubdivisionUrlRequestTest,
    SchedulesPeriodicFetchAfterRetryingHttpInternalServerErrorResponseStatusCode) {
  // Arrange
  const test::URLResponseMap url_responses = {
      {BuildSubdivisionUrlPath(),
       {{net::HTTP_INTERNAL_SERVER_ERROR,
         std::string(
             net::GetHttpReasonPhrase(net::HTTP_INTERNAL_SERVER_ERROR))},
        {net::HTTP_OK,
         test::BuildSubdivisionUrlResponseBody(/*country_code=*/"US",
                                               /*subdivision_code=*/"CA")}}}};
  test::MockUrlResponses(ads_client_mock_, url_responses);

  // Act
  subdivision_url_request_->PeriodicallyFetch();
  FastForwardClockToNextPendingTask();

  // Assert
  EXPECT_TRUE(HasPendingTasks());
}

TEST_F(BraveAdsSubdivisionUrlRequestTest,
       DoNotFetchIfAlreadyPeriodicallyFetching) {
  // Arrange
  const test::URLResponseMap url_responses = {
      {BuildSubdivisionUrlPath(),
       {{net::HTTP_OK,
         test::BuildSubdivisionUrlResponseBody(/*country_code=*/"US",
                                               /*subdivision_code=*/"CA")}}}};
  test::MockUrlResponses(ads_client_mock_, url_responses);

  subdivision_url_request_->PeriodicallyFetch();
  const size_t expected_pending_task_count = GetPendingTaskCount();

  // Act
  subdivision_url_request_->PeriodicallyFetch();

  // Assert
  EXPECT_EQ(expected_pending_task_count, GetPendingTaskCount());
}

TEST_F(BraveAdsSubdivisionUrlRequestTest,
       SchedulesRetryAfterMalformedResponseBody) {
  // Arrange
  const test::URLResponseMap url_responses = {
      {BuildSubdivisionUrlPath(),
       {{net::HTTP_OK, /*response_body=*/"malformed"}}}};
  test::MockUrlResponses(ads_client_mock_, url_responses);

  // Act
  subdivision_url_request_->PeriodicallyFetch();

  // Assert
  EXPECT_TRUE(HasPendingTasks());
}

}  // namespace brave_ads
