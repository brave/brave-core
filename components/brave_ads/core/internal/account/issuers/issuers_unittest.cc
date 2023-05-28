/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/account/issuers/issuers.h"

#include <memory>

#include "brave/components/brave_ads/common/pref_names.h"
#include "brave/components/brave_ads/core/internal/account/issuers/issuers_delegate_mock.h"
#include "brave/components/brave_ads/core/internal/account/issuers/issuers_info.h"
#include "brave/components/brave_ads/core/internal/account/issuers/issuers_unittest_util.h"
#include "brave/components/brave_ads/core/internal/account/issuers/issuers_url_request_builder_util.h"
#include "brave/components/brave_ads/core/internal/account/issuers/issuers_util.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_base.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_mock_util.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_pref_util.h"
#include "net/http/http_status_code.h"
#include "third_party/abseil-cpp/absl/types/optional.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

using ::testing::Invoke;
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

TEST_F(BraveAdsIssuersTest,
       FetchIssuersWhenEnabledPrefDidChangeIfBravePrivateAdsAreEnabled) {
  // Arrange
  SetDefaultBooleanPref(prefs::kEnabled, false);

  ASSERT_FALSE(GetIssuers());

  const URLResponseMap url_responses = {
      {BuildIssuersUrlPath(), {{net::HTTP_OK, BuildIssuersUrlResponseBody()}}}};
  MockUrlResponses(ads_client_mock_, url_responses);

  EXPECT_CALL(issuers_delegate_mock_, OnDidFetchIssuers(BuildIssuers()));

  // Act
  ads_client_mock_.SetBooleanPref(prefs::kEnabled, true);

  // Assert
}

TEST_F(BraveAdsIssuersTest,
       ResetIssuersWhenEnabledPrefDidChangeIfBravePrivateAdsAreDisabled) {
  // Arrange
  const URLResponseMap url_responses = {
      {BuildIssuersUrlPath(), {{net::HTTP_OK, BuildIssuersUrlResponseBody()}}}};
  MockUrlResponses(ads_client_mock_, url_responses);

  ON_CALL(issuers_delegate_mock_, OnDidFetchIssuers)
      .WillByDefault(
          Invoke([](const IssuersInfo& issuers) { SetIssuers(issuers); }));

  issuers_->MaybeFetch();
  ASSERT_TRUE(GetIssuers());

  // Act
  ads_client_mock_.SetBooleanPref(prefs::kEnabled, false);

  // Assert
  EXPECT_FALSE(GetIssuers());
}

TEST_F(BraveAdsIssuersTest, FetchIssuers) {
  // Arrange
  const URLResponseMap url_responses = {
      {BuildIssuersUrlPath(), {{net::HTTP_OK, BuildIssuersUrlResponseBody()}}}};
  MockUrlResponses(ads_client_mock_, url_responses);

  EXPECT_CALL(issuers_delegate_mock_, OnDidFetchIssuers(BuildIssuers()));
  EXPECT_CALL(issuers_delegate_mock_, OnFailedToFetchIssuers).Times(0);
  EXPECT_CALL(issuers_delegate_mock_, OnWillRetryFetchingIssuers).Times(0);
  EXPECT_CALL(issuers_delegate_mock_, OnDidRetryFetchingIssuers).Times(0);

  // Act
  issuers_->MaybeFetch();

  // Assert
}

TEST_F(BraveAdsIssuersTest, DoNotFetchIssuersIfInvalidJsonResponseBody) {
  // Arrange
  const URLResponseMap url_responses = {
      {BuildIssuersUrlPath(), {{net::HTTP_OK, /*response_body*/ "{INVALID}"}}}};
  MockUrlResponses(ads_client_mock_, url_responses);

  EXPECT_CALL(issuers_delegate_mock_, OnDidFetchIssuers).Times(0);
  EXPECT_CALL(issuers_delegate_mock_, OnFailedToFetchIssuers);
  EXPECT_CALL(issuers_delegate_mock_, OnWillRetryFetchingIssuers);
  EXPECT_CALL(issuers_delegate_mock_, OnDidRetryFetchingIssuers).Times(0);

  // Act
  issuers_->MaybeFetch();

  // Assert
  EXPECT_FALSE(GetIssuers());
}

TEST_F(BraveAdsIssuersTest, RetryFetchingIssuersIfNonHttpOkResponse) {
  // Arrange
  const URLResponseMap url_responses = {
      {BuildIssuersUrlPath(),
       {{net::HTTP_INTERNAL_SERVER_ERROR,
         /*response_body*/ net::GetHttpReasonPhrase(
             net::HTTP_INTERNAL_SERVER_ERROR)},
        {net::HTTP_OK, BuildIssuersUrlResponseBody()}}}};
  MockUrlResponses(ads_client_mock_, url_responses);

  EXPECT_CALL(issuers_delegate_mock_, OnDidFetchIssuers);
  EXPECT_CALL(issuers_delegate_mock_, OnFailedToFetchIssuers);
  EXPECT_CALL(issuers_delegate_mock_, OnWillRetryFetchingIssuers);
  EXPECT_CALL(issuers_delegate_mock_, OnDidRetryFetchingIssuers);

  ON_CALL(issuers_delegate_mock_, OnDidFetchIssuers)
      .WillByDefault(
          Invoke([](const IssuersInfo& issuers) { SetIssuers(issuers); }));

  // Act
  issuers_->MaybeFetch();

  FastForwardClockToNextPendingTask();

  // Assert
  EXPECT_TRUE(GetIssuers());
}

}  // namespace brave_ads
