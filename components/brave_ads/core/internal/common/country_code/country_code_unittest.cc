/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/common/country_code/country_code.h"

#include <memory>

#include "brave/components/brave_ads/core/internal/ads_client/ads_client_util.h"
#include "brave/components/brave_ads/core/internal/common/subdivision/subdivision.h"
#include "brave/components/brave_ads/core/internal/common/subdivision/url_request/subdivision_url_request_builder_util.h"
#include "brave/components/brave_ads/core/internal/common/subdivision/url_request/subdivision_url_request_test_util.h"
#include "brave/components/brave_ads/core/internal/common/test/mock_test_util.h"
#include "brave/components/brave_ads/core/internal/common/test/test_base.h"
#include "brave/components/brave_ads/core/internal/settings/settings_test_util.h"
#include "brave/components/brave_rewards/common/pref_names.h"
#include "brave/components/l10n/common/prefs.h"
#include "net/http/http_status_code.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

namespace {

std::optional<std::string> GetCountryCode() {
  return GetLocalStateStringPref(brave_l10n::prefs::kCountryCode);
}

}  // namespace

class BraveAdsCountryCodeTest : public test::TestBase {
 public:
  void SetUp() override {
    test::TestBase::SetUp();

    country_code_ = std::make_unique<CountryCode>();
    subdivision_ = std::make_unique<Subdivision>();
    subdivision_->AddObserver(&*country_code_);
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
  const brave_l10n::test::ScopedDefaultLocale scoped_default_locale_{"xx_XX"};
  std::unique_ptr<Subdivision> subdivision_;
  std::unique_ptr<CountryCode> country_code_;
};

TEST_F(BraveAdsCountryCodeTest, OnDidInitializeAds) {
  // Arrange
  MockHttpOkUrlResponse(/*country_code=*/"CA", /*subdivision_code=*/"AL");

  // Act
  NotifyDidInitializeAds();

  // Assert
  EXPECT_EQ(GetCountryCode(), "CA");
}

TEST_F(BraveAdsCountryCodeTest, PrefsNotEnabledOnDidInitializeAds) {
  // Arrange
  test::DisableBraveRewards();

  MockHttpOkUrlResponse(/*country_code=*/"CA", /*subdivision_code=*/"AL");

  // Act
  NotifyDidInitializeAds();

  // Assert
  EXPECT_EQ(GetCountryCode(), "XX");
}

TEST_F(BraveAdsCountryCodeTest, OnDidJoinBraveRewards) {
  // Arrange
  test::DisableBraveRewards();

  MockHttpOkUrlResponse(/*country_code=*/"CA", /*subdivision_code=*/"AL");

  // Act
  SetProfileBooleanPref(brave_rewards::prefs::kEnabled, true);

  // Assert
  EXPECT_EQ(GetCountryCode(), "CA");
}

TEST_F(BraveAdsCountryCodeTest, OnDidChangePrefOutside) {
  // Arrange
  test::DisableBraveRewards();

  SetLocalStateStringPref(brave_l10n::prefs::kCountryCode, "CA");

  // Act
  SetProfileBooleanPref(brave_rewards::prefs::kEnabled, true);

  // Assert
  EXPECT_EQ(GetCountryCode(), "CA");
}

TEST_F(BraveAdsCountryCodeTest, PrefWasChangedBefore) {
  // Arrange
  test::DisableBraveRewards();

  SetLocalStateStringPref(brave_l10n::prefs::kCountryCode, "CA");

  MockHttpOkUrlResponse(/*country_code=*/"XX", /*subdivision_code=*/"sd");

  // Act
  SetProfileBooleanPref(brave_rewards::prefs::kEnabled, true);

  // Assert
  EXPECT_EQ(GetCountryCode(), "XX");
}

TEST_F(BraveAdsCountryCodeTest, RetryAfterInvalidUrlResponseStatusCode) {
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
  EXPECT_EQ(GetCountryCode(), "US");
}

TEST_F(BraveAdsCountryCodeTest, NoRegionSubdivisionCode) {
  // Arrange
  MockHttpOkUrlResponse(/*country_code=*/"US",
                        /*subdivision_code=*/"NO REGION");

  // Act
  NotifyDidInitializeAds();

  // Assert
  EXPECT_EQ(GetCountryCode(), "US");
}

TEST_F(BraveAdsCountryCodeTest, EmptySubdivisionCode) {
  // Arrange
  MockHttpOkUrlResponse(/*country_code=*/"US", /*subdivision_code=*/"");

  // Act
  NotifyDidInitializeAds();

  // Assert
  EXPECT_EQ(GetCountryCode(), "XX");
}

TEST_F(BraveAdsCountryCodeTest, EmptyCountryCode) {
  // Arrange
  MockHttpOkUrlResponse(/*country_code=*/"", /*subdivision_code=*/"CA");

  // Act
  NotifyDidInitializeAds();

  // Assert
  EXPECT_EQ(GetCountryCode(), "XX");
}

TEST_F(BraveAdsCountryCodeTest, NotValidSubdivisionResonse) {
  // Arrange
  const test::URLResponseMap url_responses = {
      {BuildSubdivisionUrlPath(), {{net::HTTP_OK, /*response_body=*/"{}"}}}};
  test::MockUrlResponses(ads_client_mock_, url_responses);

  // Act
  NotifyDidInitializeAds();

  // Assert
  EXPECT_EQ(GetCountryCode(), "XX");
}

}  // namespace brave_ads
