/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/geographic/subdivision_targeting/subdivision_targeting.h"

#include <memory>

#include "brave/components/brave_ads/common/pref_names.h"
#include "brave/components/brave_ads/core/internal/ads/ad_unittest_util.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_base.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_mock_util.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_pref_util.h"
#include "brave/components/brave_ads/core/internal/geographic/subdivision_targeting/subdivision_url_request_builder_util.h"
#include "brave/components/brave_ads/core/internal/geographic/subdivision_targeting/subdivision_url_request_unittest_util.h"
#include "brave/components/brave_news/common/pref_names.h"
#include "brave/components/l10n/common/test/scoped_default_locale.h"
#include "net/http/http_status_code.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

class BraveAdsSubdivisionTargetingTest : public UnitTestBase {
 protected:
  void SetUp() override {
    UnitTestBase::SetUp();

    subdivision_targeting_ = std::make_unique<SubdivisionTargeting>();
  }

  void MockHttpOkUrlResponse(const std::string& country,
                             const std::string& region) {
    const URLResponseMap url_responses = {
        {BuildSubdivisionUrlPath(),
         {{net::HTTP_OK,
           BuildSubdivisionUrlResponseBodyForTesting(country, region)}}}};
    MockUrlResponses(ads_client_mock_, url_responses);
  }

  std::unique_ptr<SubdivisionTargeting> subdivision_targeting_;
};

TEST_F(BraveAdsSubdivisionTargetingTest,
       AllowAndFetchWhenOptingInToNotificationAds) {
  // Arrange
  DisableBraveNewsAdsForTesting();
  DisableNotificationAdsForTesting();

  MockHttpOkUrlResponse(/*country*/ "US", /*region*/ "CA");

  // Act
  ads_client_mock_.SetBooleanPref(prefs::kOptedInToNotificationAds, true);

  // Assert
  EXPECT_TRUE(SubdivisionTargeting::ShouldAllow());
  EXPECT_FALSE(subdivision_targeting_->IsDisabled());
  EXPECT_TRUE(subdivision_targeting_->ShouldAutoDetect());
  EXPECT_EQ("US-CA", ads_client_mock_.GetStringPref(
                         prefs::kSubdivisionTargetingAutoDetectedSubdivision));
}

TEST_F(BraveAdsSubdivisionTargetingTest,
       AllowAndFetchWhenOptingInToBraveNewsAds) {
  // Arrange
  DisableBraveNewsAdsForTesting();
  DisableNotificationAdsForTesting();

  MockHttpOkUrlResponse(/*country*/ "US", /*region*/ "CA");

  // Act
  ads_client_mock_.SetBooleanPref(brave_news::prefs::kBraveNewsOptedIn, true);
  ads_client_mock_.SetBooleanPref(brave_news::prefs::kNewTabPageShowToday,
                                  true);

  // Assert
  EXPECT_TRUE(SubdivisionTargeting::ShouldAllow());
  EXPECT_FALSE(subdivision_targeting_->IsDisabled());
  EXPECT_TRUE(subdivision_targeting_->ShouldAutoDetect());
  EXPECT_EQ("US-CA", ads_client_mock_.GetStringPref(
                         prefs::kSubdivisionTargetingAutoDetectedSubdivision));
}

TEST_F(BraveAdsSubdivisionTargetingTest,
       DoNotFetchWhenOptingOutOfNotificationAds) {
  // Arrange
  DisableBraveNewsAdsForTesting();

  MockHttpOkUrlResponse(/*country*/ "US", /*region*/ "CA");

  NotifyDidInitializeAds();

  EXPECT_CALL(ads_client_mock_, UrlRequest).Times(0);

  // Act
  ads_client_mock_.SetBooleanPref(prefs::kOptedInToNotificationAds, false);

  // Assert
}

TEST_F(BraveAdsSubdivisionTargetingTest,
       DoNotFetchWhenOptingOutOfBraveNewsAds) {
  // Arrange
  DisableNotificationAdsForTesting();

  MockHttpOkUrlResponse(/*country*/ "US", /*region*/ "CA");

  NotifyDidInitializeAds();

  EXPECT_CALL(ads_client_mock_, UrlRequest).Times(0);

  // Act
  ads_client_mock_.SetBooleanPref(brave_news::prefs::kBraveNewsOptedIn, false);
  ads_client_mock_.SetBooleanPref(brave_news::prefs::kNewTabPageShowToday,
                                  false);

  // Assert
}

TEST_F(BraveAdsSubdivisionTargetingTest,
       ShouldAllowAndAutoDetectForSupportedCountryAndRegionUrlResponse) {
  // Arrange
  MockHttpOkUrlResponse(/*country*/ "US", /*region*/ "CA");

  // Act
  NotifyDidInitializeAds();

  // Assert
  EXPECT_TRUE(SubdivisionTargeting::ShouldAllow());
  EXPECT_FALSE(subdivision_targeting_->IsDisabled());
  EXPECT_TRUE(subdivision_targeting_->ShouldAutoDetect());
  EXPECT_EQ("US-CA", ads_client_mock_.GetStringPref(
                         prefs::kSubdivisionTargetingAutoDetectedSubdivision));
}

TEST_F(
    BraveAdsSubdivisionTargetingTest,
    ShouldAllowButDefaultToDisabledForSupportedCountryButNoRegionUrlResponse) {
  // Arrange
  MockHttpOkUrlResponse(/*country*/ "US", /*region*/ "NO REGION");

  // Act
  NotifyDidInitializeAds();

  // Assert
  EXPECT_TRUE(SubdivisionTargeting::ShouldAllow());
  EXPECT_TRUE(subdivision_targeting_->IsDisabled());
  EXPECT_FALSE(subdivision_targeting_->ShouldAutoDetect());
}

TEST_F(BraveAdsSubdivisionTargetingTest,
       ShouldAutoDetectForUnsupportedCountryAndRegionUrlResponse) {
  // Arrange
  MockHttpOkUrlResponse(/*country*/ "XX", /*region*/ "XX");

  // Act
  NotifyDidInitializeAds();

  // Assert
  EXPECT_FALSE(SubdivisionTargeting::ShouldAllow());
  EXPECT_FALSE(subdivision_targeting_->IsDisabled());
  EXPECT_TRUE(subdivision_targeting_->ShouldAutoDetect());
}

TEST_F(BraveAdsSubdivisionTargetingTest, ShouldAutoDetectForUnsupportedLocale) {
  // Arrange
  const brave_l10n::test::ScopedDefaultLocale scoped_default_locale{"xx_XX"};

  // Act
  NotifyDidInitializeAds();

  // Assert
  EXPECT_FALSE(SubdivisionTargeting::ShouldAllow());
  EXPECT_FALSE(subdivision_targeting_->IsDisabled());
  EXPECT_TRUE(subdivision_targeting_->ShouldAutoDetect());
}

TEST_F(BraveAdsSubdivisionTargetingTest,
       ShouldAllowIfDisabledAndCountryIsSupported) {
  // Arrange
  SetDefaultStringPref(prefs::kSubdivisionTargetingSubdivision, "DISABLED");

  // Act
  NotifyDidInitializeAds();

  // Assert
  EXPECT_TRUE(SubdivisionTargeting::ShouldAllow());
  EXPECT_TRUE(subdivision_targeting_->IsDisabled());
  EXPECT_FALSE(subdivision_targeting_->ShouldAutoDetect());
}

TEST_F(BraveAdsSubdivisionTargetingTest,
       ShouldAllowAndAutoDetectIfCountryIsSupported) {
  // Arrange
  SetDefaultStringPref(prefs::kSubdivisionTargetingAutoDetectedSubdivision,
                       "US-CA");

  // Act
  NotifyDidInitializeAds();

  // Assert
  EXPECT_TRUE(SubdivisionTargeting::ShouldAllow());
  EXPECT_FALSE(subdivision_targeting_->IsDisabled());
  EXPECT_TRUE(subdivision_targeting_->ShouldAutoDetect());
  EXPECT_EQ("US-CA", ads_client_mock_.GetStringPref(
                         prefs::kSubdivisionTargetingAutoDetectedSubdivision));
}

TEST_F(BraveAdsSubdivisionTargetingTest, ShouldNotAllowIfCountryIsUnsupported) {
  // Arrange
  const brave_l10n::test::ScopedDefaultLocale scoped_default_locale{"xx_XX"};

  // Act
  NotifyDidInitializeAds();

  // Assert
  EXPECT_FALSE(SubdivisionTargeting::ShouldAllow());
  EXPECT_FALSE(subdivision_targeting_->IsDisabled());
  EXPECT_TRUE(subdivision_targeting_->ShouldAutoDetect());
}

TEST_F(BraveAdsSubdivisionTargetingTest,
       ShouldNotAllowIfLocaleAndSubdivisionCountriesMismatch) {
  // Arrange
  SetDefaultStringPref(prefs::kSubdivisionTargetingSubdivision, "CA-QC");

  // Act
  NotifyDidInitializeAds();

  // Assert
  EXPECT_FALSE(SubdivisionTargeting::ShouldAllow());
  EXPECT_FALSE(subdivision_targeting_->IsDisabled());
  EXPECT_TRUE(subdivision_targeting_->ShouldAutoDetect());
}

TEST_F(BraveAdsSubdivisionTargetingTest,
       ShouldAutoDetectAndNotAllowIfSubdivisionCodeIsEmpty) {
  // Arrange

  // Act
  NotifyDidInitializeAds();

  // Assert
  EXPECT_FALSE(SubdivisionTargeting::ShouldAllow());
  EXPECT_FALSE(subdivision_targeting_->IsDisabled());
  EXPECT_TRUE(subdivision_targeting_->ShouldAutoDetect());
}

TEST_F(BraveAdsSubdivisionTargetingTest,
       RetryAfterInvalidUrlResponseStatusCode) {
  // Arrange
  const URLResponseMap url_responses = {
      {BuildSubdivisionUrlPath(),
       {{net::HTTP_INTERNAL_SERVER_ERROR,
         /*response_body*/ net::GetHttpReasonPhrase(
             net::HTTP_INTERNAL_SERVER_ERROR)},
        {net::HTTP_OK, BuildSubdivisionUrlResponseBodyForTesting(
                           /*country*/ "US", /*region*/ "CA")}}}};
  MockUrlResponses(ads_client_mock_, url_responses);

  NotifyDidInitializeAds();

  // Act
  FastForwardClockToNextPendingTask();

  // Assert
  EXPECT_TRUE(SubdivisionTargeting::ShouldAllow());
  EXPECT_FALSE(subdivision_targeting_->IsDisabled());
  EXPECT_EQ("US-CA", ads_client_mock_.GetStringPref(
                         prefs::kSubdivisionTargetingAutoDetectedSubdivision));
}

class BraveAdsSubdivisionTargetingRetryOnInvalidUrlResponseBodyTest
    : public BraveAdsSubdivisionTargetingTest,
      public ::testing::WithParamInterface<const char*> {};

TEST_P(BraveAdsSubdivisionTargetingRetryOnInvalidUrlResponseBodyTest,
       RetryAfterInvalidUrlResponseBody) {
  // Arrange
  const URLResponseMap url_responses = {
      {BuildSubdivisionUrlPath(),
       {{net::HTTP_OK, /*response_body*/ GetParam()},
        {net::HTTP_OK, BuildSubdivisionUrlResponseBodyForTesting(
                           /*country*/ "US", /*region*/ "CA")}}}};
  MockUrlResponses(ads_client_mock_, url_responses);

  NotifyDidInitializeAds();

  // Act
  FastForwardClockToNextPendingTask();

  // Assert
  EXPECT_TRUE(SubdivisionTargeting::ShouldAllow());
  EXPECT_FALSE(subdivision_targeting_->IsDisabled());
  EXPECT_EQ("US-CA", ads_client_mock_.GetStringPref(
                         prefs::kSubdivisionTargetingAutoDetectedSubdivision));
}

INSTANTIATE_TEST_SUITE_P(
    ,
    BraveAdsSubdivisionTargetingRetryOnInvalidUrlResponseBodyTest,
    ::testing::Values("",
                      "INVALID",
                      "{}",
                      "{INVALID}",
                      R"({"country":"US","region":""})",
                      R"({"country":"","region":"CA"})",
                      R"({"country":"","region":""})",
                      R"({"country":"US"})",
                      R"({"region":"CA"})"));

}  // namespace brave_ads
