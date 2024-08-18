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
#include "brave/components/brave_news/common/pref_names.h"
#include "brave/components/brave_rewards/common/pref_names.h"
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

TEST_F(BraveAdsSubdivisionTest, PrefsNotEnabledOnDidInitializeAds) {
  // Arrange
  test::DisableBraveRewards();
  test::OptOutOfBraveNewsAds();

  MockHttpOkUrlResponse(/*country_code=*/"US", /*subdivision_code=*/"CA");

  EXPECT_CALL(subdivision_observer_mock_, OnDidUpdateSubdivision).Times(0);

  // Act
  NotifyDidInitializeAds();

  // Assert
  EXPECT_FALSE(HasPendingTasks());
}

TEST_F(BraveAdsSubdivisionTest, OnDidJoinBraveRewards) {
  // Arrange
  test::DisableBraveRewards();
  test::OptOutOfBraveNewsAds();

  MockHttpOkUrlResponse(/*country_code=*/"US", /*subdivision_code=*/"CA");

  EXPECT_CALL(subdivision_observer_mock_, OnDidUpdateSubdivision("US-CA"));

  // Act
  SetProfileBooleanPref(brave_rewards::prefs::kEnabled, true);

  // Assert
  EXPECT_TRUE(HasPendingTasks());
}

TEST_F(BraveAdsSubdivisionTest,
       FetchWhenOptingInToBraveNewsIfBraveRewardsIsDisabled) {
  // Arrange
  test::DisableBraveRewards();
  test::OptOutOfBraveNewsAds();

  MockHttpOkUrlResponse(/*country_code=*/"US", /*subdivision_code=*/"CA");

  EXPECT_CALL(subdivision_observer_mock_, OnDidUpdateSubdivision("US-CA"));

  // Act
  SetProfileBooleanPref(brave_news::prefs::kBraveNewsOptedIn, true);
  SetProfileBooleanPref(brave_news::prefs::kNewTabPageShowToday, true);

  // Assert
  EXPECT_TRUE(HasPendingTasks());
}

TEST_F(BraveAdsSubdivisionTest, OnDidResetBraveRewards) {
  // Arrange
  test::OptOutOfBraveNewsAds();

  MockHttpOkUrlResponse(/*country_code=*/"US", /*subdivision_code=*/"CA");

  NotifyDidInitializeAds();

  ASSERT_TRUE(HasPendingTasks());

  // Act
  SetProfileBooleanPref(brave_rewards::prefs::kEnabled, false);

  // Assert
  EXPECT_FALSE(HasPendingTasks());
}

TEST_F(BraveAdsSubdivisionTest, OnDidOptOutBraveNews) {
  // Arrange
  test::DisableBraveRewards();

  MockHttpOkUrlResponse(/*country_code=*/"US", /*subdivision_code=*/"CA");

  EXPECT_CALL(subdivision_observer_mock_, OnDidUpdateSubdivision("US-CA"));

  NotifyDidInitializeAds();

  ASSERT_TRUE(HasPendingTasks());

  // Act
  SetProfileBooleanPref(brave_news::prefs::kNewTabPageShowToday, false);

  // Assert
  EXPECT_FALSE(HasPendingTasks());
}

TEST_F(BraveAdsSubdivisionTest, RetryAfterInvalidUrlResponseStatusCode) {
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

TEST_F(BraveAdsSubdivisionTest, NotValidSubdivisionResonse) {
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

}  // namespace brave_ads
