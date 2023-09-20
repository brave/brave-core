/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/geographical/subdivision/subdivision.h"

#include "brave/components/brave_ads/core/internal/common/unittest/unittest_base.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_mock_util.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_url_response_alias.h"
#include "brave/components/brave_ads/core/internal/geographical/subdivision/subdivision_observer.h"
#include "brave/components/brave_ads/core/internal/geographical/subdivision/subdivision_url_request_builder_util.h"
#include "brave/components/brave_ads/core/internal/geographical/subdivision/subdivision_url_request_unittest_util.h"
#include "brave/components/brave_ads/core/internal/settings/settings_unittest_util.h"
#include "brave/components/brave_news/common/pref_names.h"
#include "brave/components/brave_rewards/common/pref_names.h"
#include "net/http/http_status_code.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

namespace {

using testing::_;

class TestSubdivisionObserver : public SubdivisionObserver {
 public:
  MOCK_METHOD1(OnDidUpdateSubdivision, void(const std::string&));
};

}  // namespace

class BraveAdsSubdivisionTest : public UnitTestBase {
 public:
  void SetUp() override {
    UnitTestBase::SetUp();

    subdivision_ = std::make_unique<Subdivision>();
  }

  void MockHttpOkUrlResponse(const std::string& country_code,
                             const std::string& subdivision_code) {
    const URLResponseMap url_responses = {
        {BuildSubdivisionUrlPath(),
         {{net::HTTP_OK, BuildSubdivisionUrlResponseBodyForTesting(
                             country_code, subdivision_code)}}}};
    MockUrlResponses(ads_client_mock_, url_responses);
  }

 protected:
  std::unique_ptr<Subdivision> subdivision_;
};

TEST_F(BraveAdsSubdivisionTest, OnDidInitializeAds) {
  // Arrange
  MockHttpOkUrlResponse(/*country_code*/ "US", /*subdivision_code*/ "CA");

  TestSubdivisionObserver observer;
  EXPECT_CALL(observer, OnDidUpdateSubdivision("US-CA"));
  subdivision_->AddObserver(&observer);

  // Act
  NotifyDidInitializeAds();

  // Assert
  EXPECT_TRUE(subdivision_->get_is_periodically_fetching_for_testing());
}

TEST_F(BraveAdsSubdivisionTest, PrefsNotEnabledOnDidInitializeAds) {
  // Arrange
  DisableBraveNewsAdsForTesting();
  DisableBraveRewardsForTesting();

  MockHttpOkUrlResponse(/*country_code*/ "US", /*subdivision_code*/ "CA");

  TestSubdivisionObserver observer;
  EXPECT_CALL(observer, OnDidUpdateSubdivision(_)).Times(0);
  subdivision_->AddObserver(&observer);

  // Act
  NotifyDidInitializeAds();

  // Assert
  EXPECT_FALSE(subdivision_->get_is_periodically_fetching_for_testing());
}

TEST_F(BraveAdsSubdivisionTest, OnDidJoinBraveRewards) {
  // Arrange
  DisableBraveNewsAdsForTesting();
  DisableBraveRewardsForTesting();

  MockHttpOkUrlResponse(/*country_code*/ "US", /*subdivision_code*/ "CA");

  TestSubdivisionObserver observer;
  EXPECT_CALL(observer, OnDidUpdateSubdivision("US-CA"));
  subdivision_->AddObserver(&observer);

  // Act
  ads_client_mock_.SetBooleanPref(brave_rewards::prefs::kEnabled, true);

  // Assert
  EXPECT_TRUE(subdivision_->get_is_periodically_fetching_for_testing());
}

TEST_F(BraveAdsSubdivisionTest, OnDidOptinBraveNews) {
  // Arrange
  DisableBraveNewsAdsForTesting();
  DisableBraveRewardsForTesting();

  MockHttpOkUrlResponse(/*country_code*/ "US", /*subdivision_code*/ "CA");

  TestSubdivisionObserver observer;
  EXPECT_CALL(observer, OnDidUpdateSubdivision("US-CA"));
  subdivision_->AddObserver(&observer);

  // Act
  ads_client_mock_.SetBooleanPref(brave_news::prefs::kBraveNewsOptedIn, true);
  ads_client_mock_.SetBooleanPref(brave_news::prefs::kNewTabPageShowToday,
                                  true);

  // Assert
  EXPECT_TRUE(subdivision_->get_is_periodically_fetching_for_testing());
}

TEST_F(BraveAdsSubdivisionTest, OnDidResetBraveRewards) {
  // Arrange
  DisableBraveNewsAdsForTesting();

  MockHttpOkUrlResponse(/*country_code*/ "US", /*subdivision_code*/ "CA");

  NotifyDidInitializeAds();

  EXPECT_TRUE(subdivision_->get_is_periodically_fetching_for_testing());

  // Act
  ads_client_mock_.SetBooleanPref(brave_rewards::prefs::kEnabled, false);

  // Assert
  EXPECT_FALSE(subdivision_->get_is_periodically_fetching_for_testing());
}

TEST_F(BraveAdsSubdivisionTest, OnDidOptoutBraveNews) {
  // Arrange
  DisableBraveRewardsForTesting();

  MockHttpOkUrlResponse(/*country_code*/ "US", /*subdivision_code*/ "CA");

  NotifyDidInitializeAds();

  EXPECT_TRUE(subdivision_->get_is_periodically_fetching_for_testing());

  // Act
  ads_client_mock_.SetBooleanPref(brave_news::prefs::kNewTabPageShowToday,
                                  false);

  // Assert
  EXPECT_FALSE(subdivision_->get_is_periodically_fetching_for_testing());
}

TEST_F(BraveAdsSubdivisionTest, RetryAfterInvalidUrlResponseStatusCode) {
  // Arrange
  const URLResponseMap url_responses = {
      {BuildSubdivisionUrlPath(),
       {{net::HTTP_INTERNAL_SERVER_ERROR,
         /*response_body*/ net::GetHttpReasonPhrase(
             net::HTTP_INTERNAL_SERVER_ERROR)},
        {net::HTTP_OK,
         BuildSubdivisionUrlResponseBodyForTesting(
             /*country_code*/ "US", /*subdivision_code*/ "CA")}}}};
  MockUrlResponses(ads_client_mock_, url_responses);

  TestSubdivisionObserver observer;
  EXPECT_CALL(observer, OnDidUpdateSubdivision("US-CA"));
  subdivision_->AddObserver(&observer);

  NotifyDidInitializeAds();

  // Act
  FastForwardClockToNextPendingTask();

  // Assert
  EXPECT_TRUE(subdivision_->get_is_periodically_fetching_for_testing());
}

TEST_F(BraveAdsSubdivisionTest, NoRegionSubdivisionCode) {
  // Arrange
  MockHttpOkUrlResponse(/*country_code*/ "US",
                        /*subdivision_code*/ "NO REGION");

  TestSubdivisionObserver observer;
  EXPECT_CALL(observer, OnDidUpdateSubdivision("US-NO REGION"));
  subdivision_->AddObserver(&observer);

  // Act
  NotifyDidInitializeAds();

  // Assert
  EXPECT_TRUE(subdivision_->get_is_periodically_fetching_for_testing());
}

TEST_F(BraveAdsSubdivisionTest, EmptySubdivisionCode) {
  // Arrange
  MockHttpOkUrlResponse(/*country_code*/ "US", /*subdivision_code*/ "");

  TestSubdivisionObserver observer;
  EXPECT_CALL(observer, OnDidUpdateSubdivision(_)).Times(0);
  subdivision_->AddObserver(&observer);

  // Act
  NotifyDidInitializeAds();

  // Assert
  EXPECT_TRUE(subdivision_->get_is_periodically_fetching_for_testing());
}

TEST_F(BraveAdsSubdivisionTest, EmptyRegionCode) {
  // Arrange
  MockHttpOkUrlResponse(/*country_code*/ "", /*subdivision_code*/ "CA");

  TestSubdivisionObserver observer;
  EXPECT_CALL(observer, OnDidUpdateSubdivision(_)).Times(0);
  subdivision_->AddObserver(&observer);

  // Act
  NotifyDidInitializeAds();

  // Assert
  EXPECT_TRUE(subdivision_->get_is_periodically_fetching_for_testing());
}

TEST_F(BraveAdsSubdivisionTest, NotValidSubdivisionResonse) {
  // Arrange
  const URLResponseMap url_responses = {
      {BuildSubdivisionUrlPath(), {{net::HTTP_OK, "{}"}}}};
  MockUrlResponses(ads_client_mock_, url_responses);

  TestSubdivisionObserver observer;
  EXPECT_CALL(observer, OnDidUpdateSubdivision(_)).Times(0);
  subdivision_->AddObserver(&observer);

  // Act
  NotifyDidInitializeAds();

  // Assert
  EXPECT_TRUE(subdivision_->get_is_periodically_fetching_for_testing());
}

}  // namespace brave_ads
