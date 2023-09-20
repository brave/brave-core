/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/geographical/region/region.h"

#include "base/values.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_base.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_mock_util.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_url_response_alias.h"
#include "brave/components/brave_ads/core/internal/geographical/subdivision/subdivision.h"
#include "brave/components/brave_ads/core/internal/geographical/subdivision/subdivision_url_request_builder_util.h"
#include "brave/components/brave_ads/core/internal/geographical/subdivision/subdivision_url_request_unittest_util.h"
#include "brave/components/brave_ads/core/internal/settings/settings_unittest_util.h"
#include "brave/components/brave_rewards/common/pref_names.h"
#include "brave/components/l10n/common/prefs.h"
#include "net/http/http_status_code.h"
#include "third_party/abseil-cpp/absl/types/optional.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

class BraveAdsRegionTest : public UnitTestBase {
 public:
  void SetUp() override {
    UnitTestBase::SetUp();

    region_ = std::make_unique<Region>();
    subdivision_ = std::make_unique<Subdivision>();
    subdivision_->AddObserver(region_.get());
  }

  void MockHttpOkUrlResponse(const std::string& country_code,
                             const std::string& subdivision_code) {
    const URLResponseMap url_responses = {
        {BuildSubdivisionUrlPath(),
         {{net::HTTP_OK, BuildSubdivisionUrlResponseBodyForTesting(
                             country_code, subdivision_code)}}}};
    MockUrlResponses(ads_client_mock_, url_responses);
  }

  absl::optional<std::string> GetGeoRegionCode() const {
    auto pref_value =
        ads_client_mock_.GetLocalStatePref(brave_l10n::prefs::kGeoRegionCode);
    if (!pref_value || !pref_value->is_string()) {
      return absl::nullopt;
    }
    return pref_value->GetString();
  }

 protected:
  const brave_l10n::test::ScopedDefaultLocale scoped_default_locale_{"xx_XX"};
  std::unique_ptr<Subdivision> subdivision_;
  std::unique_ptr<Region> region_;
};

TEST_F(BraveAdsRegionTest, OnDidInitializeAds) {
  // Arrange
  MockHttpOkUrlResponse(/*country_code*/ "CA", /*subdivision_code*/ "AL");

  // Act
  NotifyDidInitializeAds();

  // Assert
  EXPECT_EQ(GetGeoRegionCode(), "CA");
}

TEST_F(BraveAdsRegionTest, PrefsNotEnabledOnDidInitializeAds) {
  // Arrange
  DisableBraveRewardsForTesting();

  MockHttpOkUrlResponse(/*country_code*/ "CA", /*subdivision_code*/ "AL");

  // Act
  NotifyDidInitializeAds();

  // Assert
  EXPECT_EQ(GetGeoRegionCode(), "XX");
}

TEST_F(BraveAdsRegionTest, OnDidJoinBraveRewards) {
  // Arrange
  DisableBraveRewardsForTesting();

  MockHttpOkUrlResponse(/*country_code*/ "CA", /*subdivision_code*/ "AL");

  // Act
  ads_client_mock_.SetBooleanPref(brave_rewards::prefs::kEnabled, true);

  // Assert
  EXPECT_EQ(GetGeoRegionCode(), "CA");
}

TEST_F(BraveAdsRegionTest, OnDidChangePrefOutside) {
  // Arrange
  DisableBraveRewardsForTesting();

  ads_client_mock_.SetLocalStatePref(brave_l10n::prefs::kGeoRegionCode,
                                     base::Value("CA"));

  // Act
  ads_client_mock_.SetBooleanPref(brave_rewards::prefs::kEnabled, true);

  // Assert
  EXPECT_EQ(GetGeoRegionCode(), "CA");
}

TEST_F(BraveAdsRegionTest, RetryAfterInvalidUrlResponseStatusCode) {
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

  NotifyDidInitializeAds();

  // Act
  FastForwardClockToNextPendingTask();

  // Assert
  EXPECT_EQ(GetGeoRegionCode(), "US");
}

TEST_F(BraveAdsRegionTest, NoRegionSubdivisionCode) {
  // Arrange
  MockHttpOkUrlResponse(/*country_code*/ "US",
                        /*subdivision_code*/ "NO REGION");

  // Act
  NotifyDidInitializeAds();

  // Assert
  EXPECT_EQ(GetGeoRegionCode(), "US");
}

TEST_F(BraveAdsRegionTest, EmptySubdivisionCode) {
  // Arrange
  MockHttpOkUrlResponse(/*country_code*/ "US", /*subdivision_code*/ "");

  // Act
  NotifyDidInitializeAds();

  // Assert
  EXPECT_EQ(GetGeoRegionCode(), "XX");
}

TEST_F(BraveAdsRegionTest, EmptyRegionCode) {
  // Arrange
  MockHttpOkUrlResponse(/*country_code*/ "", /*subdivision_code*/ "CA");

  // Act
  NotifyDidInitializeAds();

  // Assert
  EXPECT_EQ(GetGeoRegionCode(), "XX");
}

TEST_F(BraveAdsRegionTest, NotValidSubdivisionResonse) {
  // Arrange
  const URLResponseMap url_responses = {
      {BuildSubdivisionUrlPath(), {{net::HTTP_OK, "{}"}}}};
  MockUrlResponses(ads_client_mock_, url_responses);

  // Act
  NotifyDidInitializeAds();

  // Assert
  EXPECT_EQ(GetGeoRegionCode(), "XX");
}

}  // namespace brave_ads
