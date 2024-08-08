/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/serving/eligible_ads/exclusion_rules/subdivision_targeting_exclusion_rule.h"

#include <memory>

#include "base/strings/strcat.h"
#include "base/strings/string_util.h"
#include "brave/components/brave_ads/core/internal/ad_units/ad_test_constants.h"
#include "brave/components/brave_ads/core/internal/common/subdivision/subdivision.h"
#include "brave/components/brave_ads/core/internal/common/subdivision/subdivision_test_util.h"
#include "brave/components/brave_ads/core/internal/common/subdivision/url_request/subdivision_url_request_builder_util.h"
#include "brave/components/brave_ads/core/internal/common/subdivision/url_request/subdivision_url_request_test_util.h"
#include "brave/components/brave_ads/core/internal/common/test/mock_test_util.h"
#include "brave/components/brave_ads/core/internal/common/test/profile_pref_value_test_util.h"
#include "brave/components/brave_ads/core/internal/common/test/test_base.h"
#include "brave/components/brave_ads/core/internal/creatives/creative_ad_info.h"
#include "brave/components/brave_ads/core/internal/targeting/geographical/subdivision/subdivision_targeting.h"
#include "brave/components/brave_ads/core/public/prefs/pref_names.h"
#include "brave/components/l10n/common/test/scoped_default_locale.h"
#include "net/http/http_status_code.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

namespace {

struct ParamInfo final {
  const char* const country_code;
  const char* const subdivision_code;
} constexpr kTests[] = {{.country_code = "US", .subdivision_code = "AL"},
                        {.country_code = "US", .subdivision_code = "AK"},
                        {.country_code = "US", .subdivision_code = "AZ"},
                        {.country_code = "US", .subdivision_code = "AR"},
                        {.country_code = "US", .subdivision_code = "CA"},
                        {.country_code = "US", .subdivision_code = "CO"},
                        {.country_code = "US", .subdivision_code = "CT"},
                        {.country_code = "US", .subdivision_code = "DE"},
                        {.country_code = "US", .subdivision_code = "FL"},
                        {.country_code = "US", .subdivision_code = "GA"},
                        {.country_code = "US", .subdivision_code = "HI"},
                        {.country_code = "US", .subdivision_code = "ID"},
                        {.country_code = "US", .subdivision_code = "IL"},
                        {.country_code = "US", .subdivision_code = "IN"},
                        {.country_code = "US", .subdivision_code = "IA"},
                        {.country_code = "US", .subdivision_code = "KS"},
                        {.country_code = "US", .subdivision_code = "KY"},
                        {.country_code = "US", .subdivision_code = "LA"},
                        {.country_code = "US", .subdivision_code = "ME"},
                        {.country_code = "US", .subdivision_code = "MD"},
                        {.country_code = "US", .subdivision_code = "MA"},
                        {.country_code = "US", .subdivision_code = "MI"},
                        {.country_code = "US", .subdivision_code = "MN"},
                        {.country_code = "US", .subdivision_code = "MS"},
                        {.country_code = "US", .subdivision_code = "MO"},
                        {.country_code = "US", .subdivision_code = "MT"},
                        {.country_code = "US", .subdivision_code = "NE"},
                        {.country_code = "US", .subdivision_code = "NV"},
                        {.country_code = "US", .subdivision_code = "NH"},
                        {.country_code = "US", .subdivision_code = "NJ"},
                        {.country_code = "US", .subdivision_code = "NM"},
                        {.country_code = "US", .subdivision_code = "NY"},
                        {.country_code = "US", .subdivision_code = "NC"},
                        {.country_code = "US", .subdivision_code = "ND"},
                        {.country_code = "US", .subdivision_code = "OH"},
                        {.country_code = "US", .subdivision_code = "OK"},
                        {.country_code = "US", .subdivision_code = "OR"},
                        {.country_code = "US", .subdivision_code = "PA"},
                        {.country_code = "US", .subdivision_code = "RI"},
                        {.country_code = "US", .subdivision_code = "SC"},
                        {.country_code = "US", .subdivision_code = "SD"},
                        {.country_code = "US", .subdivision_code = "TN"},
                        {.country_code = "US", .subdivision_code = "TX"},
                        {.country_code = "US", .subdivision_code = "UT"},
                        {.country_code = "US", .subdivision_code = "VT"},
                        {.country_code = "US", .subdivision_code = "VA"},
                        {.country_code = "US", .subdivision_code = "WA"},
                        {.country_code = "US", .subdivision_code = "WV"},
                        {.country_code = "US", .subdivision_code = "WI"},
                        {.country_code = "US", .subdivision_code = "WY"},
                        {.country_code = "CA", .subdivision_code = "AB"},
                        {.country_code = "CA", .subdivision_code = "BC"},
                        {.country_code = "CA", .subdivision_code = "MB"},
                        {.country_code = "CA", .subdivision_code = "NB"},
                        {.country_code = "CA", .subdivision_code = "NS"},
                        {.country_code = "CA", .subdivision_code = "NT"},
                        {.country_code = "CA", .subdivision_code = "NU"},
                        {.country_code = "CA", .subdivision_code = "ON"},
                        {.country_code = "CA", .subdivision_code = "PE"},
                        {.country_code = "CA", .subdivision_code = "QC"},
                        {.country_code = "CA", .subdivision_code = "SK"},
                        {.country_code = "CA", .subdivision_code = "YT"}};

}  // namespace

class BraveAdsSubdivisionTargetingExclusionRuleTest
    : public test::TestBase,
      public ::testing::WithParamInterface<ParamInfo> {
 protected:
  void SetUp() override {
    test::TestBase::SetUp();

    scoped_default_locale_ =
        std::make_unique<brave_l10n::test::ScopedDefaultLocale>(
            base::StrCat({"en", "_", GetParam().country_code}));

    subdivision_targeting_ = std::make_unique<SubdivisionTargeting>();
    subdivision_ = std::make_unique<Subdivision>();
    subdivision_->AddObserver(&*subdivision_targeting_);
    exclusion_rule_ = std::make_unique<SubdivisionTargetingExclusionRule>(
        *subdivision_targeting_);
  }

  static std::string BuildSubdivisionForTestParam() {
    return test::BuildSubdivision(GetParam().country_code,
                                  GetParam().subdivision_code);
  }

  static std::string BuildOtherSubdivisionForTestParam() {
    const char* subdivision_code = "";

    if (GetParam().country_code ==
        std::string_view("US") /*United States of America*/) {
      if (GetParam().subdivision_code == std::string_view("FL") /*Florida*/) {
        subdivision_code = "CA";  // Alabama
      } else {
        subdivision_code = "FL";  // Florida
      }
    } else if (GetParam().country_code == std::string_view("CA") /*Canada*/) {
      if (GetParam().subdivision_code == std::string_view("QC") /*Quebec*/) {
        subdivision_code = "AB";  // Alberta
      } else {
        subdivision_code = "QC";  // Quebec
      }
    }

    return test::BuildSubdivision(GetParam().country_code, subdivision_code);
  }

  void MockUrlResponseForTestParam() {
    const test::URLResponseMap url_responses = {
        {BuildSubdivisionUrlPath(),
         {{net::HTTP_OK,
           test::BuildSubdivisionUrlResponseBody(
               GetParam().country_code, GetParam().subdivision_code)}}}};
    test::MockUrlResponses(ads_client_mock_, url_responses);
  }

  std::unique_ptr<brave_l10n::test::ScopedDefaultLocale> scoped_default_locale_;

  std::unique_ptr<SubdivisionTargeting> subdivision_targeting_;
  std::unique_ptr<Subdivision> subdivision_;
  std::unique_ptr<SubdivisionTargetingExclusionRule> exclusion_rule_;
};

TEST_P(
    BraveAdsSubdivisionTargetingExclusionRuleTest,
    ShouldExcludeIfSubdivisionTargetingIsNotAllowedForGeoTargetWithSubdivisionCode) {
  // Arrange
  CreativeAdInfo creative_ad;
  creative_ad.creative_set_id = test::kCreativeSetId;
  creative_ad.geo_targets = {BuildSubdivisionForTestParam()};

  // Act & Assert
  EXPECT_FALSE(exclusion_rule_->ShouldInclude(creative_ad).has_value());
}

TEST_P(
    BraveAdsSubdivisionTargetingExclusionRuleTest,
    ShouldIncludeIfSubdivisionTargetingIsNotAllowedForGeoTargetWithNoSubdivisionCode) {
  // Arrange
  test::SetProfileBooleanPrefValue(prefs::kShouldAllowSubdivisionTargeting,
                                   false);

  CreativeAdInfo creative_ad;
  creative_ad.creative_set_id = test::kCreativeSetId;
  creative_ad.geo_targets = {GetParam().country_code};

  // Act & Assert
  EXPECT_TRUE(exclusion_rule_->ShouldInclude(creative_ad).has_value());
}

TEST_P(BraveAdsSubdivisionTargetingExclusionRuleTest,
       ShouldIncludeIfSubdivisionTargetingIsSupportedAndAutoDetected) {
  // Arrange
  test::SetProfileBooleanPrefValue(prefs::kShouldAllowSubdivisionTargeting,
                                   true);

  MockUrlResponseForTestParam();

  NotifyDidInitializeAds();

  CreativeAdInfo creative_ad;
  creative_ad.creative_set_id = test::kCreativeSetId;
  creative_ad.geo_targets = {BuildSubdivisionForTestParam()};

  // Act & Assert
  EXPECT_TRUE(exclusion_rule_->ShouldInclude(creative_ad).has_value());
}

TEST_P(
    BraveAdsSubdivisionTargetingExclusionRuleTest,
    ShouldIncludeIfSubdivisionTargetingIsSupportedAndAutoDetectedForMultipleGeoTargets) {
  // Arrange
  test::SetProfileBooleanPrefValue(prefs::kShouldAllowSubdivisionTargeting,
                                   true);

  MockUrlResponseForTestParam();

  NotifyDidInitializeAds();

  CreativeAdInfo creative_ad;
  creative_ad.creative_set_id = test::kCreativeSetId;
  creative_ad.geo_targets = {BuildSubdivisionForTestParam(),
                             BuildOtherSubdivisionForTestParam()};

  // Act & Assert
  EXPECT_TRUE(exclusion_rule_->ShouldInclude(creative_ad).has_value());
}

TEST_P(
    BraveAdsSubdivisionTargetingExclusionRuleTest,
    ShouldIncludeIfSubdivisionTargetingIsSupportedAndAutoDetectedForGeoTargetWithNoSubdivisionCode) {
  // Arrange
  test::SetProfileBooleanPrefValue(prefs::kShouldAllowSubdivisionTargeting,
                                   true);

  MockUrlResponseForTestParam();

  NotifyDidInitializeAds();

  CreativeAdInfo creative_ad;
  creative_ad.creative_set_id = test::kCreativeSetId;
  creative_ad.geo_targets = {GetParam().country_code};

  // Act & Assert
  EXPECT_TRUE(exclusion_rule_->ShouldInclude(creative_ad).has_value());
}

TEST_P(
    BraveAdsSubdivisionTargetingExclusionRuleTest,
    ShouldIncludeIfSubdivisionTargetingIsSupportedAndSubdivisionWasManuallySelected) {
  // Arrange
  test::SetProfileBooleanPrefValue(prefs::kShouldAllowSubdivisionTargeting,
                                   true);

  test::SetProfileStringPrefValue(
      prefs::kSubdivisionTargetingUserSelectedSubdivision,
      BuildSubdivisionForTestParam());

  MockUrlResponseForTestParam();

  NotifyDidInitializeAds();

  CreativeAdInfo creative_ad;
  creative_ad.creative_set_id = test::kCreativeSetId;
  creative_ad.geo_targets = {BuildSubdivisionForTestParam()};

  // Act & Assert
  EXPECT_TRUE(exclusion_rule_->ShouldInclude(creative_ad).has_value());
}

TEST_P(
    BraveAdsSubdivisionTargetingExclusionRuleTest,
    ShouldIncludeIfSubdivisionTargetingIsSupportedAndSubdivisionWasManuallySelectedForMultipleGeoTargets) {
  // Arrange
  test::SetProfileBooleanPrefValue(prefs::kShouldAllowSubdivisionTargeting,
                                   true);

  test::SetProfileStringPrefValue(
      prefs::kSubdivisionTargetingUserSelectedSubdivision,
      BuildSubdivisionForTestParam());

  MockUrlResponseForTestParam();

  NotifyDidInitializeAds();

  CreativeAdInfo creative_ad;
  creative_ad.creative_set_id = test::kCreativeSetId;
  creative_ad.geo_targets = {BuildSubdivisionForTestParam(),
                             BuildOtherSubdivisionForTestParam()};

  // Act & Assert
  EXPECT_TRUE(exclusion_rule_->ShouldInclude(creative_ad).has_value());
}

TEST_P(
    BraveAdsSubdivisionTargetingExclusionRuleTest,
    ShouldIncludeIfSubdivisionTargetingIsSupportedAndManuallySelectedForGeoTargetWithNoSubdivisionCode) {
  // Arrange
  test::SetProfileBooleanPrefValue(prefs::kShouldAllowSubdivisionTargeting,
                                   true);

  test::SetProfileStringPrefValue(
      prefs::kSubdivisionTargetingUserSelectedSubdivision,
      BuildSubdivisionForTestParam());

  MockUrlResponseForTestParam();

  NotifyDidInitializeAds();

  CreativeAdInfo creative_ad;
  creative_ad.creative_set_id = test::kCreativeSetId;
  creative_ad.geo_targets = {GetParam().country_code};

  // Act & Assert
  EXPECT_TRUE(exclusion_rule_->ShouldInclude(creative_ad).has_value());
}

TEST_P(BraveAdsSubdivisionTargetingExclusionRuleTest,
       ShouldExcludeIfSubdivisionTargetingIsSupportedForUnsupportedGeoTarget) {
  // Arrange
  test::SetProfileBooleanPrefValue(prefs::kShouldAllowSubdivisionTargeting,
                                   true);

  MockUrlResponseForTestParam();

  NotifyDidInitializeAds();

  CreativeAdInfo creative_ad;
  creative_ad.creative_set_id = test::kCreativeSetId;
  creative_ad.geo_targets = {test::BuildSubdivision(
      /*country_code=*/"US", /*subdivision_code=*/"XX")};

  // Assert
  EXPECT_FALSE(exclusion_rule_->ShouldInclude(creative_ad).has_value());
}

TEST_P(
    BraveAdsSubdivisionTargetingExclusionRuleTest,
    ShouldExcludeIfSubdivisionTargetingIsNotSupportedForSubdivisionGeoTarget) {
  // Arrange
  test::SetProfileBooleanPrefValue(prefs::kShouldAllowSubdivisionTargeting,
                                   true);

  MockUrlResponseForTestParam();

  NotifyDidInitializeAds();

  CreativeAdInfo creative_ad;
  creative_ad.creative_set_id = test::kCreativeSetId;
  creative_ad.geo_targets = {test::BuildSubdivision(
      /*country_code=*/"GB", /*subdivision_code=*/"DEV")};

  // Act & Assert
  EXPECT_FALSE(exclusion_rule_->ShouldInclude(creative_ad).has_value());
}

TEST_P(
    BraveAdsSubdivisionTargetingExclusionRuleTest,
    ShouldIncludeIfSubdivisionTargetingIsNotSupportedForNonSubdivisionGeoTarget) {
  // Arrange
  test::SetProfileBooleanPrefValue(prefs::kShouldAllowSubdivisionTargeting,
                                   true);

  const test::URLResponseMap url_responses = {
      {BuildSubdivisionUrlPath(),
       {{net::HTTP_OK,
         test::BuildSubdivisionUrlResponseBody(
             /*country_code=*/"XX", /*subdivision_code=*/"NO REGION")}}}};
  test::MockUrlResponses(ads_client_mock_, url_responses);

  NotifyDidInitializeAds();

  CreativeAdInfo creative_ad;
  creative_ad.creative_set_id = test::kCreativeSetId;
  creative_ad.geo_targets = {"XX"};

  // Act & Assert
  EXPECT_TRUE(exclusion_rule_->ShouldInclude(creative_ad).has_value());
}

TEST_P(BraveAdsSubdivisionTargetingExclusionRuleTest,
       ShouldExcludeIfSubdivisionTargetingIsDisabledForSubdivisionGeoTarget) {
  // Arrange
  test::SetProfileBooleanPrefValue(prefs::kShouldAllowSubdivisionTargeting,
                                   true);

  test::SetProfileStringPrefValue(
      prefs::kSubdivisionTargetingUserSelectedSubdivision, "DISABLED");

  MockUrlResponseForTestParam();

  NotifyDidInitializeAds();

  CreativeAdInfo creative_ad;
  creative_ad.creative_set_id = test::kCreativeSetId;
  creative_ad.geo_targets = {BuildSubdivisionForTestParam()};

  // Act & Assert
  EXPECT_FALSE(exclusion_rule_->ShouldInclude(creative_ad).has_value());
}

TEST_P(
    BraveAdsSubdivisionTargetingExclusionRuleTest,
    ShouldIncludeIfSubdivisionTargetingIsDisabledForNonSubdivisionGeoTarget) {
  // Arrange
  test::SetProfileBooleanPrefValue(prefs::kShouldAllowSubdivisionTargeting,
                                   true);

  test::SetProfileStringPrefValue(
      prefs::kSubdivisionTargetingUserSelectedSubdivision, "DISABLED");

  MockUrlResponseForTestParam();

  NotifyDidInitializeAds();

  CreativeAdInfo creative_ad;
  creative_ad.creative_set_id = test::kCreativeSetId;
  creative_ad.geo_targets = {"XX"};

  // Act & Assert
  EXPECT_TRUE(exclusion_rule_->ShouldInclude(creative_ad).has_value());
}

std::string TestParamToString(::testing::TestParamInfo<ParamInfo> test_param) {
  return base::ReplaceStringPlaceholders(
      "CountryCode$1SubdivisionCode$2",
      {test_param.param.country_code, test_param.param.subdivision_code},
      nullptr);
}

INSTANTIATE_TEST_SUITE_P(,
                         BraveAdsSubdivisionTargetingExclusionRuleTest,
                         ::testing::ValuesIn(kTests),
                         TestParamToString);

}  // namespace brave_ads
