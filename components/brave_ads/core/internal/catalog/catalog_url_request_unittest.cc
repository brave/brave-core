/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/catalog/catalog_url_request.h"

#include <memory>
#include <optional>
#include <string>

#include "brave/components/brave_ads/core/internal/catalog/catalog_test_constants.h"
#include "brave/components/brave_ads/core/internal/catalog/catalog_url_request_builder_util.h"
#include "brave/components/brave_ads/core/internal/catalog/test/catalog_url_request_delegate_mock.h"
#include "brave/components/brave_ads/core/internal/common/test/file_test_util.h"
#include "brave/components/brave_ads/core/internal/common/test/mock_test_util.h"
#include "brave/components/brave_ads/core/internal/common/test/test_base.h"
#include "net/http/http_status_code.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

class BraveAdsCatalogUrlRequestTest : public test::TestBase {
 protected:
  void SetUp() override {
    test::TestBase::SetUp();

    catalog_url_request_ = std::make_unique<CatalogUrlRequest>();
    catalog_url_request_->SetDelegate(&delegate_mock_);
  }

  std::unique_ptr<CatalogUrlRequest> catalog_url_request_;
  ::testing::NiceMock<CatalogUrlRequestDelegateMock> delegate_mock_;
};

TEST_F(BraveAdsCatalogUrlRequestTest,
       SchedulesPeriodicFetchAfterHttpOkResponseStatusCode) {
  // Arrange
  const std::optional<std::string> response_body =
      test::MaybeReadFileToStringAndReplaceTags(
          test::kCatalogWithSingleCampaignJsonFilename);
  ASSERT_TRUE(response_body);

  const test::URLResponseMap url_responses = {
      {BuildCatalogUrlPath(), {{net::HTTP_OK, *response_body}}}};
  test::MockUrlResponses(ads_client_mock_, url_responses);

  // Act
  catalog_url_request_->PeriodicallyFetch();

  // Assert
  EXPECT_TRUE(HasPendingTasks());
}

TEST_F(BraveAdsCatalogUrlRequestTest,
       SchedulesRetryAfterHttpInternalServerErrorResponseStatusCode) {
  // Arrange
  const test::URLResponseMap url_responses = {
      {BuildCatalogUrlPath(),
       {{net::HTTP_INTERNAL_SERVER_ERROR,
         std::string(
             net::GetHttpReasonPhrase(net::HTTP_INTERNAL_SERVER_ERROR))}}}};
  test::MockUrlResponses(ads_client_mock_, url_responses);

  // Act
  catalog_url_request_->PeriodicallyFetch();

  // Assert
  EXPECT_TRUE(HasPendingTasks());
}

TEST_F(BraveAdsCatalogUrlRequestTest,
       DoNotRetryAfterHttpUnauthorizedResponseStatusCode) {
  // Arrange
  const test::URLResponseMap url_responses = {
      {BuildCatalogUrlPath(),
       {{net::HTTP_UNAUTHORIZED,
         std::string(net::GetHttpReasonPhrase(net::HTTP_UNAUTHORIZED))}}}};
  test::MockUrlResponses(ads_client_mock_, url_responses);

  // Act
  catalog_url_request_->PeriodicallyFetch();

  // Assert
  EXPECT_FALSE(HasPendingTasks());
}

TEST_F(BraveAdsCatalogUrlRequestTest,
       DoNotRetryAfterHttpForbiddenResponseStatusCode) {
  // Arrange
  const test::URLResponseMap url_responses = {
      {BuildCatalogUrlPath(),
       {{net::HTTP_FORBIDDEN,
         std::string(net::GetHttpReasonPhrase(net::HTTP_FORBIDDEN))}}}};
  test::MockUrlResponses(ads_client_mock_, url_responses);

  // Act
  catalog_url_request_->PeriodicallyFetch();

  // Assert
  EXPECT_FALSE(HasPendingTasks());
}

TEST_F(BraveAdsCatalogUrlRequestTest,
       DoNotRetryAfterHttpTooManyRequestsResponseStatusCode) {
  // Arrange
  const test::URLResponseMap url_responses = {
      {BuildCatalogUrlPath(),
       {{net::HTTP_TOO_MANY_REQUESTS,
         std::string(net::GetHttpReasonPhrase(net::HTTP_TOO_MANY_REQUESTS))}}}};
  test::MockUrlResponses(ads_client_mock_, url_responses);

  // Act
  catalog_url_request_->PeriodicallyFetch();

  // Assert
  EXPECT_FALSE(HasPendingTasks());
}

TEST_F(
    BraveAdsCatalogUrlRequestTest,
    SchedulesPeriodicFetchAfterRetryingHttpInternalServerErrorResponseStatusCode) {
  // Arrange
  const std::optional<std::string> response_body =
      test::MaybeReadFileToStringAndReplaceTags(
          test::kCatalogWithSingleCampaignJsonFilename);
  ASSERT_TRUE(response_body);

  const test::URLResponseMap url_responses = {
      {BuildCatalogUrlPath(),
       {{net::HTTP_INTERNAL_SERVER_ERROR,
         std::string(
             net::GetHttpReasonPhrase(net::HTTP_INTERNAL_SERVER_ERROR))},
        {net::HTTP_OK, *response_body}}}};
  test::MockUrlResponses(ads_client_mock_, url_responses);

  // Act
  catalog_url_request_->PeriodicallyFetch();
  FastForwardClockToNextPendingTask();

  // Assert
  EXPECT_TRUE(HasPendingTasks());
}

TEST_F(BraveAdsCatalogUrlRequestTest, DoNotFetchIfAlreadyPeriodicallyFetching) {
  // Arrange
  const std::optional<std::string> response_body =
      test::MaybeReadFileToStringAndReplaceTags(
          test::kCatalogWithSingleCampaignJsonFilename);
  ASSERT_TRUE(response_body);

  const test::URLResponseMap url_responses = {
      {BuildCatalogUrlPath(), {{net::HTTP_OK, *response_body}}}};
  test::MockUrlResponses(ads_client_mock_, url_responses);

  catalog_url_request_->PeriodicallyFetch();
  const size_t expected_pending_task_count = GetPendingTaskCount();

  // Act
  catalog_url_request_->PeriodicallyFetch();

  // Assert
  EXPECT_EQ(expected_pending_task_count, GetPendingTaskCount());
}

TEST_F(BraveAdsCatalogUrlRequestTest,
       SchedulesRetryAfterMalformedResponseBody) {
  // Arrange
  const test::URLResponseMap url_responses = {
      {BuildCatalogUrlPath(), {{net::HTTP_OK, /*response_body=*/"malformed"}}}};
  test::MockUrlResponses(ads_client_mock_, url_responses);

  // Act
  catalog_url_request_->PeriodicallyFetch();

  // Assert
  EXPECT_TRUE(HasPendingTasks());
}

TEST_F(BraveAdsCatalogUrlRequestTest,
       SchedulesRetryAfterCatalogVersionMismatch) {
  // Arrange
  const test::URLResponseMap url_responses = {{BuildCatalogUrlPath(),
                                               {{net::HTTP_OK,
                                                 R"(
            {
              "catalogId": "29e5c8bc0ba319069980bb390d8e8f9b58c05a20",
              "version": 0,
              "ping": 7200000,
              "campaigns": []
            }
          )"}}}};
  test::MockUrlResponses(ads_client_mock_, url_responses);

  // Act
  catalog_url_request_->PeriodicallyFetch();

  // Assert
  EXPECT_TRUE(HasPendingTasks());
}

}  // namespace brave_ads
