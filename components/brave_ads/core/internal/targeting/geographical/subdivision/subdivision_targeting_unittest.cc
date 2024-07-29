/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/targeting/geographical/subdivision/subdivision_targeting.h"

#include <memory>

#include "brave/components/brave_ads/core/internal/ads_client/ads_client_util.h"
#include "brave/components/brave_ads/core/internal/common/subdivision/subdivision.h"
#include "brave/components/brave_ads/core/internal/common/subdivision/url_request/subdivision_url_request_builder_util.h"
#include "brave/components/brave_ads/core/internal/common/subdivision/url_request/subdivision_url_request_test_util.h"
#include "brave/components/brave_ads/core/internal/common/test/mock_test_util.h"
#include "brave/components/brave_ads/core/internal/common/test/profile_pref_value_test_util.h"
#include "brave/components/brave_ads/core/internal/common/test/test_base.h"
#include "brave/components/brave_ads/core/internal/settings/settings_test_util.h"
#include "brave/components/brave_ads/core/internal/targeting/geographical/subdivision/subdivision_targeting_constants.h"
#include "brave/components/brave_ads/core/public/prefs/pref_names.h"
#include "brave/components/brave_news/common/pref_names.h"
#include "brave/components/l10n/common/test/scoped_default_locale.h"
#include "net/http/http_status_code.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

class BraveAdsSubdivisionTargetingTest : public test::TestBase {
 protected:
  void SetUp() override {
    test::TestBase::SetUp();

    subdivision_targeting_ = std::make_unique<SubdivisionTargeting>();
    subdivision_ = std::make_unique<Subdivision>();
    subdivision_->AddObserver(&*subdivision_targeting_);
  }

  void MockHttpOkUrlResponse(const std::string& country_code,
                             const std::string& subdivision_code) {
    const test::URLResponseMap url_responses = {
        {BuildSubdivisionUrlPath(),
         {{net::HTTP_OK, test::BuildSubdivisionUrlResponseBody(
                             country_code, subdivision_code)}}}};
    test::MockUrlResponses(ads_client_mock_, url_responses);
  }

  std::unique_ptr<SubdivisionTargeting> subdivision_targeting_;
  std::unique_ptr<Subdivision> subdivision_;
};

TEST_F(BraveAdsSubdivisionTargetingTest,
       AllowAndFetchWhenOptingInToNotificationAds) {
  // Arrange
  test::OptOutOfBraveNewsAds();
  test::OptOutOfNotificationAds();

  MockHttpOkUrlResponse(/*country_code=*/"US", /*subdivision_code=*/"CA");

  NotifyDidInitializeAds();

  // Act
  SetProfileBooleanPref(prefs::kOptedInToNotificationAds, true);

  // Assert
  EXPECT_TRUE(SubdivisionTargeting::ShouldAllow());
  EXPECT_FALSE(subdivision_targeting_->IsDisabled());
  EXPECT_TRUE(subdivision_targeting_->ShouldAutoDetect());
  EXPECT_EQ("US-CA", GetProfileStringPref(
                         prefs::kSubdivisionTargetingAutoDetectedSubdivision));
}

TEST_F(BraveAdsSubdivisionTargetingTest,
       AllowAndFetchWhenOptingInToBraveNewsAds) {
  // Arrange
  test::OptOutOfBraveNewsAds();
  test::OptOutOfNotificationAds();

  MockHttpOkUrlResponse(/*country_code=*/"US", /*subdivision_code=*/"CA");

  NotifyDidInitializeAds();

  // Act
  SetProfileBooleanPref(brave_news::prefs::kBraveNewsOptedIn, true);
  SetProfileBooleanPref(brave_news::prefs::kNewTabPageShowToday, true);

  // Assert
  EXPECT_TRUE(SubdivisionTargeting::ShouldAllow());
  EXPECT_FALSE(subdivision_targeting_->IsDisabled());
  EXPECT_TRUE(subdivision_targeting_->ShouldAutoDetect());
  EXPECT_EQ("US-CA", GetProfileStringPref(
                         prefs::kSubdivisionTargetingAutoDetectedSubdivision));
}

TEST_F(BraveAdsSubdivisionTargetingTest,
       DoNotFetchWhenOptingOutOfNotificationAds) {
  // Arrange
  test::OptOutOfBraveNewsAds();

  MockHttpOkUrlResponse(/*country_code=*/"US", /*subdivision_code=*/"CA");

  NotifyDidInitializeAds();

  // Act & Assert
  EXPECT_CALL(ads_client_mock_, UrlRequest).Times(0);
  SetProfileBooleanPref(prefs::kOptedInToNotificationAds, false);
}

TEST_F(BraveAdsSubdivisionTargetingTest,
       DoNotFetchWhenOptingOutOfBraveNewsAds) {
  // Arrange
  test::OptOutOfNotificationAds();

  MockHttpOkUrlResponse(/*country_code=*/"US", /*subdivision_code=*/"CA");

  NotifyDidInitializeAds();

  // Act & Assert
  EXPECT_CALL(ads_client_mock_, UrlRequest).Times(0);
  SetProfileBooleanPref(brave_news::prefs::kBraveNewsOptedIn, false);
  SetProfileBooleanPref(brave_news::prefs::kNewTabPageShowToday, false);
}

TEST_F(BraveAdsSubdivisionTargetingTest,
       ShouldAllowAndAutoDetectForSupportedCountryAndRegionUrlResponse) {
  // Arrange
  MockHttpOkUrlResponse(/*country_code=*/"US", /*subdivision_code=*/"CA");

  // Act
  NotifyDidInitializeAds();

  // Assert
  EXPECT_TRUE(SubdivisionTargeting::ShouldAllow());
  EXPECT_FALSE(subdivision_targeting_->IsDisabled());
  EXPECT_TRUE(subdivision_targeting_->ShouldAutoDetect());
  EXPECT_EQ("US-CA", GetProfileStringPref(
                         prefs::kSubdivisionTargetingAutoDetectedSubdivision));
}

TEST_F(
    BraveAdsSubdivisionTargetingTest,
    ShouldAllowButDefaultToDisabledForSupportedCountryButNoRegionUrlResponse) {
  // Arrange
  MockHttpOkUrlResponse(/*country_code=*/"US",
                        /*subdivision_code=*/"NO REGION");

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
  MockHttpOkUrlResponse(/*country_code=*/"XX", /*subdivision_code=*/"XX");

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
  test::SetProfileStringPrefValue(prefs::kSubdivisionTargetingSubdivision,
                                  kSubdivisionTargetingDisabled);
  MockHttpOkUrlResponse(/*country_code=*/"US", /*subdivision_code=*/"CA");

  // Act
  NotifyDidInitializeAds();

  // Assert
  EXPECT_TRUE(SubdivisionTargeting::ShouldAllow());
  EXPECT_TRUE(subdivision_targeting_->IsDisabled());
  EXPECT_FALSE(subdivision_targeting_->ShouldAutoDetect());
}

TEST_F(BraveAdsSubdivisionTargetingTest,
       ShouldAllowIfDisabledAndCountryIsUnsupported) {
  // Arrange
  test::SetProfileStringPrefValue(prefs::kSubdivisionTargetingSubdivision,
                                  kSubdivisionTargetingDisabled);
  MockHttpOkUrlResponse(/*country_code=*/"XX", /*subdivision_code=*/"XX");

  // Act
  NotifyDidInitializeAds();

  // Assert
  EXPECT_FALSE(SubdivisionTargeting::ShouldAllow());
  EXPECT_TRUE(subdivision_targeting_->IsDisabled());
  EXPECT_FALSE(subdivision_targeting_->ShouldAutoDetect());
}

TEST_F(BraveAdsSubdivisionTargetingTest,
       ShouldAllowAndAutoDetectIfCountryIsSupported) {
  // Arrange
  test::SetProfileStringPrefValue(
      prefs::kSubdivisionTargetingAutoDetectedSubdivision, "US-CA");

  // Act
  NotifyDidInitializeAds();

  // Assert
  EXPECT_TRUE(SubdivisionTargeting::ShouldAllow());
  EXPECT_FALSE(subdivision_targeting_->IsDisabled());
  EXPECT_TRUE(subdivision_targeting_->ShouldAutoDetect());
  EXPECT_EQ("US-CA", GetProfileStringPref(
                         prefs::kSubdivisionTargetingAutoDetectedSubdivision));
}

TEST_F(BraveAdsSubdivisionTargetingTest,
       ShouldAllowAndAutoDetectIfSubdivisionCodeIsUnsupported) {
  // Arrange
  test::SetProfileStringPrefValue(
      prefs::kSubdivisionTargetingAutoDetectedSubdivision, "XX-XX");

  // Act
  NotifyDidInitializeAds();

  // Assert
  EXPECT_FALSE(SubdivisionTargeting::ShouldAllow());
  EXPECT_FALSE(subdivision_targeting_->IsDisabled());
  EXPECT_TRUE(subdivision_targeting_->ShouldAutoDetect());
  EXPECT_EQ("XX-XX", GetProfileStringPref(
                         prefs::kSubdivisionTargetingAutoDetectedSubdivision));
}

TEST_F(BraveAdsSubdivisionTargetingTest,
       ShouldNotAllowIfLocaleIsUnsupportedAndSubdivisionCodeIsEmpty) {
  // Arrange
  const brave_l10n::test::ScopedDefaultLocale scoped_default_locale{"xx_XX"};

  // Act
  NotifyDidInitializeAds();

  // Assert
  EXPECT_FALSE(SubdivisionTargeting::ShouldAllow());
  EXPECT_FALSE(subdivision_targeting_->IsDisabled());
  EXPECT_TRUE(subdivision_targeting_->ShouldAutoDetect());
}

TEST_F(BraveAdsSubdivisionTargetingTest, ShouldAllowIfSubdivisionCodeNotValid) {
  // Arrange
  test::SetProfileStringPrefValue(
      prefs::kSubdivisionTargetingAutoDetectedSubdivision, "CA-NO REGION");

  // Act
  NotifyDidInitializeAds();

  // Assert
  EXPECT_TRUE(SubdivisionTargeting::ShouldAllow());
  EXPECT_TRUE(subdivision_targeting_->IsDisabled());
  EXPECT_FALSE(subdivision_targeting_->ShouldAutoDetect());
}

TEST_F(BraveAdsSubdivisionTargetingTest,
       ShouldAutoDetectAndNotAllowIfSubdivisionCodeIsEmpty) {
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
  const test::URLResponseMap url_responses = {
      {BuildSubdivisionUrlPath(),
       {{net::HTTP_INTERNAL_SERVER_ERROR,
         /*response_body=*/net::GetHttpReasonPhrase(
             net::HTTP_INTERNAL_SERVER_ERROR)},
        {net::HTTP_OK,
         test::BuildSubdivisionUrlResponseBody(
             /*country_code=*/"US", /*subdivision_code=*/"CA")}}}};
  test::MockUrlResponses(ads_client_mock_, url_responses);

  NotifyDidInitializeAds();

  // Act
  FastForwardClockToNextPendingTask();

  // Assert
  EXPECT_TRUE(SubdivisionTargeting::ShouldAllow());
  EXPECT_FALSE(subdivision_targeting_->IsDisabled());
  EXPECT_EQ("US-CA", GetProfileStringPref(
                         prefs::kSubdivisionTargetingAutoDetectedSubdivision));
}

TEST_F(BraveAdsSubdivisionTargetingTest,
       ShouldAllowWhenUserSelectSubdivisionWithSameCountry) {
  // Arrange
  test::SetProfileStringPrefValue(prefs::kSubdivisionTargetingSubdivision,
                                  "US-FL");
  MockHttpOkUrlResponse(/*country_code=*/"US", /*subdivision_code=*/"CA");

  // Act
  NotifyDidInitializeAds();

  // Assert
  EXPECT_TRUE(SubdivisionTargeting::ShouldAllow());
  EXPECT_FALSE(subdivision_targeting_->IsDisabled());
  EXPECT_FALSE(subdivision_targeting_->ShouldAutoDetect());
  EXPECT_EQ("US-CA", GetProfileStringPref(
                         prefs::kSubdivisionTargetingAutoDetectedSubdivision));
  EXPECT_EQ("US-FL",
            GetProfileStringPref(prefs::kSubdivisionTargetingSubdivision));
  EXPECT_EQ("US-FL", subdivision_targeting_->GetSubdivision());
}

TEST_F(BraveAdsSubdivisionTargetingTest,
       ShouldAllowWhenUserSelectSubdivisionAndAutodetectedCountryIsSupported) {
  // Arrange
  test::SetProfileStringPrefValue(prefs::kSubdivisionTargetingSubdivision,
                                  "US-FL");
  MockHttpOkUrlResponse(/*country_code=*/"CA", /*subdivision_code=*/"AL");

  // Act
  NotifyDidInitializeAds();

  // Assert
  EXPECT_TRUE(SubdivisionTargeting::ShouldAllow());
  EXPECT_FALSE(subdivision_targeting_->IsDisabled());
  EXPECT_TRUE(subdivision_targeting_->ShouldAutoDetect());
  EXPECT_EQ("CA-AL", GetProfileStringPref(
                         prefs::kSubdivisionTargetingAutoDetectedSubdivision));
}

TEST_F(
    BraveAdsSubdivisionTargetingTest,
    ShouldNotAllowWhenUserSelectSubdivisionAndAutodetectedCountryIsUnsupported) {
  // Arrange
  test::SetProfileStringPrefValue(prefs::kSubdivisionTargetingSubdivision,
                                  "US-FL");
  MockHttpOkUrlResponse(/*country_code=*/"XX", /*subdivision_code=*/"XX");

  // Act
  NotifyDidInitializeAds();

  // Assert
  EXPECT_FALSE(SubdivisionTargeting::ShouldAllow());
  EXPECT_FALSE(subdivision_targeting_->IsDisabled());
  EXPECT_FALSE(subdivision_targeting_->ShouldAutoDetect());
  EXPECT_EQ("XX-XX", GetProfileStringPref(
                         prefs::kSubdivisionTargetingAutoDetectedSubdivision));
  EXPECT_EQ("US-FL",
            GetProfileStringPref(prefs::kSubdivisionTargetingSubdivision));
  EXPECT_EQ("US-FL", subdivision_targeting_->GetSubdivision());
}

class BraveAdsSubdivisionTargetingRetryOnInvalidUrlResponseBodyTest
    : public BraveAdsSubdivisionTargetingTest,
      public ::testing::WithParamInterface<const char*> {};

TEST_P(BraveAdsSubdivisionTargetingRetryOnInvalidUrlResponseBodyTest,
       RetryAfterInvalidUrlResponseBody) {
  // Arrange
  const test::URLResponseMap url_responses = {
      {BuildSubdivisionUrlPath(),
       {{net::HTTP_OK, /*response_body=*/GetParam()},
        {net::HTTP_OK,
         test::BuildSubdivisionUrlResponseBody(
             /*country_code=*/"US", /*subdivision_code=*/"CA")}}}};
  test::MockUrlResponses(ads_client_mock_, url_responses);

  NotifyDidInitializeAds();

  // Act
  FastForwardClockToNextPendingTask();

  // Assert
  EXPECT_TRUE(SubdivisionTargeting::ShouldAllow());
  EXPECT_FALSE(subdivision_targeting_->IsDisabled());
  EXPECT_EQ("US-CA", GetProfileStringPref(
                         prefs::kSubdivisionTargetingAutoDetectedSubdivision));
}

INSTANTIATE_TEST_SUITE_P(
    ,
    BraveAdsSubdivisionTargetingRetryOnInvalidUrlResponseBodyTest,
    ::testing::Values("",
                      "MALFORMED",
                      "{}",
                      "{MALFORMED}",
                      R"({"country":"US","region":""})",
                      R"({"country":"","region":"CA"})",
                      R"({"country":"","region":""})",
                      R"({"country":"US"})",
                      R"({"region":"CA"})"));

}  // namespace brave_ads
