/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/common/subdivision/subdivision.h"

#include "brave/components/brave_ads/core/internal/common/subdivision/subdivision_observer_mock.h"
#include "brave/components/brave_ads/core/internal/common/subdivision/url_request/subdivision_url_request_builder_util.h"
#include "brave/components/brave_ads/core/internal/common/subdivision/url_request/subdivision_url_request_test_util.h"
#include "brave/components/brave_ads/core/internal/common/test/mock_test_util.h"
#include "brave/components/brave_ads/core/internal/common/test/test_base.h"
#include "brave/components/brave_ads/core/internal/prefs/pref_util.h"
#include "brave/components/brave_ads/core/internal/settings/settings_test_util.h"
#include "brave/components/brave_ads/core/public/prefs/pref_names.h"
#include "brave/components/brave_rewards/core/pref_names.h"
#include "brave/components/ntp_background_images/common/pref_names.h"
#include "net/http/http_status_code.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

class BraveAdsSubdivisionTest : public test::TestBase {
 public:
  void SetUp() override {
    test::TestBase::SetUp();

    subdivision_ = std::make_unique<Subdivision>();
    subdivision_->AddObserver(&subdivision_observer_mock_);
  }

  void TearDown() override {
    subdivision_->RemoveObserver(&subdivision_observer_mock_);

    test::TestBase::TearDown();
  }

  void MockHttpOkUrlResponse(const std::string& country_code,
                             const std::string& subdivision_code) {
    const test::URLResponseMap url_responses = {
        {BuildSubdivisionUrlPath(),
         {{net::HTTP_OK, test::BuildSubdivisionUrlResponseBody(
                             country_code, subdivision_code)}}}};
    test::MockUrlResponses(ads_client_mock_, url_responses);
  }

 protected:
  std::unique_ptr<Subdivision> subdivision_;

  SubdivisionObserverMock subdivision_observer_mock_;
};

TEST_F(BraveAdsSubdivisionTest, OnDidInitializeAds) {
  // Arrange
  MockHttpOkUrlResponse(/*country_code=*/"US", /*subdivision_code=*/"CA");

  EXPECT_CALL(subdivision_observer_mock_, OnDidUpdateSubdivision("US-CA"));

  // Act
  NotifyDidInitializeAds();

  // Assert
  EXPECT_TRUE(HasPendingTasks());
}

TEST_F(BraveAdsSubdivisionTest, FetchIfUserJoinsBraveRewards) {
  // Arrange
  test::DisableBraveRewards();

  MockHttpOkUrlResponse(/*country_code=*/"US", /*subdivision_code=*/"CA");

  EXPECT_CALL(subdivision_observer_mock_, OnDidUpdateSubdivision);

  // Act & Assert
  SetProfileBooleanPref(brave_rewards::prefs::kEnabled, true);
}

TEST_F(BraveAdsSubdivisionTest, DoNotFetchIfUserHasNotJoinedBraveRewards) {
  // Arrange
  MockHttpOkUrlResponse(/*country_code=*/"US", /*subdivision_code=*/"CA");

  EXPECT_CALL(subdivision_observer_mock_, OnDidUpdateSubdivision).Times(0);

  // Act & Assert
  SetProfileBooleanPref(brave_rewards::prefs::kEnabled, false);
}

TEST_F(BraveAdsSubdivisionTest, DoNotFetchWhenOptingOutOfNotificationAds) {
  // Arrange
  MockHttpOkUrlResponse(/*country_code=*/"US", /*subdivision_code=*/"CA");

  EXPECT_CALL(subdivision_observer_mock_, OnDidUpdateSubdivision).Times(0);

  // Act & Assert
  SetProfileBooleanPref(prefs::kOptedInToNotificationAds, false);
}

TEST_F(BraveAdsSubdivisionTest, FetchWhenOptingInToNotificationAds) {
  // Arrange
  test::OptOutOfAllAds();

  MockHttpOkUrlResponse(/*country_code=*/"US", /*subdivision_code=*/"CA");

  EXPECT_CALL(subdivision_observer_mock_, OnDidUpdateSubdivision);

  // Act & Assert
  SetProfileBooleanPref(prefs::kOptedInToNotificationAds, true);
}

TEST_F(BraveAdsSubdivisionTest, DoNotFetchWhenOptingOutOfNewTabPageAds) {
  // Arrange
  MockHttpOkUrlResponse(/*country_code=*/"US", /*subdivision_code=*/"CA");

  EXPECT_CALL(subdivision_observer_mock_, OnDidUpdateSubdivision).Times(0);

  // Act & Assert
  SetProfileBooleanPref(
      ntp_background_images::prefs::kNewTabPageShowBackgroundImage, false);
  SetProfileBooleanPref(ntp_background_images::prefs::
                            kNewTabPageShowSponsoredImagesBackgroundImage,
                        false);
}

TEST_F(BraveAdsSubdivisionTest, DoNotFetchWhenOptingInToNewTabPageAds) {
  // Arrange
  test::OptOutOfAllAds();

  MockHttpOkUrlResponse(/*country_code=*/"US", /*subdivision_code=*/"CA");

  EXPECT_CALL(subdivision_observer_mock_, OnDidUpdateSubdivision).Times(0);

  // Act & Assert
  SetProfileBooleanPref(
      ntp_background_images::prefs::kNewTabPageShowBackgroundImage, true);
  SetProfileBooleanPref(ntp_background_images::prefs::
                            kNewTabPageShowSponsoredImagesBackgroundImage,
                        true);
}

TEST_F(BraveAdsSubdivisionTest, DoNotFetchWhenOptingOutOfSearchResultAds) {
  // Arrange
  MockHttpOkUrlResponse(/*country_code=*/"US", /*subdivision_code=*/"CA");

  EXPECT_CALL(subdivision_observer_mock_, OnDidUpdateSubdivision).Times(0);

  // Act & Assert
  SetProfileBooleanPref(prefs::kOptedInToSearchResultAds, false);
}

TEST_F(BraveAdsSubdivisionTest, DoNotFetchWhenOptingInToSearchResultAds) {
  // Arrange
  test::OptOutOfAllAds();

  MockHttpOkUrlResponse(/*country_code=*/"US", /*subdivision_code=*/"CA");

  EXPECT_CALL(subdivision_observer_mock_, OnDidUpdateSubdivision).Times(0);

  // Act & Assert
  SetProfileBooleanPref(prefs::kOptedInToSearchResultAds, true);
}

TEST_F(BraveAdsSubdivisionTest,
       DoNotRetryIfHttpForbiddenUrlResponseStatusCode) {
  // Arrange
  const test::URLResponseMap url_responses = {
      {BuildSubdivisionUrlPath(),
       {{net::HTTP_FORBIDDEN,
         /*response_body=*/net::GetHttpReasonPhrase(net::HTTP_FORBIDDEN)}}}};
  test::MockUrlResponses(ads_client_mock_, url_responses);

  EXPECT_CALL(subdivision_observer_mock_, OnDidUpdateSubdivision).Times(0);

  NotifyDidInitializeAds();

  // Act
  FastForwardClockToNextPendingTask();

  // Assert
  EXPECT_TRUE(HasPendingTasks());
}

TEST_F(BraveAdsSubdivisionTest,
       RetryIfHttpInternalServerErrorResponseStatusCode) {
  // Arrange
  const test::URLResponseMap url_responses = {
      {BuildSubdivisionUrlPath(),
       {{net::HTTP_INTERNAL_SERVER_ERROR,
         /*response_body=*/net::GetHttpReasonPhrase(
             net::HTTP_INTERNAL_SERVER_ERROR)},
        {net::HTTP_OK,
         test::BuildSubdivisionUrlResponseBody(
             /*country_code=*/"US", /*subdivision_code=*/"CA")}}}};
  test::MockUrlResponses(ads_client_mock_, url_responses);

  EXPECT_CALL(subdivision_observer_mock_, OnDidUpdateSubdivision("US-CA"));

  NotifyDidInitializeAds();

  // Act
  FastForwardClockToNextPendingTask();

  // Assert
  EXPECT_TRUE(HasPendingTasks());
}

TEST_F(BraveAdsSubdivisionTest, RetryIfResponseBodyIsInvalid) {
  // Arrange
  const test::URLResponseMap url_responses = {
      {BuildSubdivisionUrlPath(), {{net::HTTP_OK, /*response_body=*/"{}"}}}};
  test::MockUrlResponses(ads_client_mock_, url_responses);

  EXPECT_CALL(subdivision_observer_mock_, OnDidUpdateSubdivision).Times(0);

  // Act
  NotifyDidInitializeAds();

  // Assert
  EXPECT_TRUE(HasPendingTasks());
}

TEST_F(BraveAdsSubdivisionTest, NoRegionSubdivisionCode) {
  // Arrange
  MockHttpOkUrlResponse(/*country_code=*/"US",
                        /*subdivision_code=*/"NO REGION");

  EXPECT_CALL(subdivision_observer_mock_,
              OnDidUpdateSubdivision("US-NO REGION"));

  // Act
  NotifyDidInitializeAds();

  // Assert
  EXPECT_TRUE(HasPendingTasks());
}

TEST_F(BraveAdsSubdivisionTest, EmptySubdivisionCode) {
  // Arrange
  MockHttpOkUrlResponse(/*country_code=*/"US", /*subdivision_code=*/"");

  EXPECT_CALL(subdivision_observer_mock_, OnDidUpdateSubdivision).Times(0);

  // Act
  NotifyDidInitializeAds();

  // Assert
  EXPECT_TRUE(HasPendingTasks());
}

TEST_F(BraveAdsSubdivisionTest, EmptyCountryCode) {
  // Arrange
  MockHttpOkUrlResponse(/*country_code=*/"", /*subdivision_code=*/"CA");

  EXPECT_CALL(subdivision_observer_mock_, OnDidUpdateSubdivision).Times(0);

  // Act
  NotifyDidInitializeAds();

  // Assert
  EXPECT_TRUE(HasPendingTasks());
}

}  // namespace brave_ads
