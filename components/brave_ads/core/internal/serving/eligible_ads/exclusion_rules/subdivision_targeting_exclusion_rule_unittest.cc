/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/serving/eligible_ads/exclusion_rules/subdivision_targeting_exclusion_rule.h"

#include <memory>

#include "base/strings/strcat.h"
#include "base/strings/string_util.h"
#include "brave/components/brave_ads/core/internal/common/subdivision/subdivision_unittest_util.h"
#include "brave/components/brave_ads/core/internal/common/subdivision/url_request/subdivision_url_request_builder_util.h"
#include "brave/components/brave_ads/core/internal/common/subdivision/url_request/subdivision_url_request_unittest_util.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_base.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_mock_util.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_pref_util.h"
#include "brave/components/brave_ads/core/internal/creatives/creative_ad_info.h"
#include "brave/components/brave_ads/core/internal/targeting/geographical/subdivision/subdivision_targeting.h"
#include "brave/components/brave_ads/core/internal/units/ad_unittest_constants.h"
#include "brave/components/brave_ads/core/public/prefs/pref_names.h"
#include "brave/components/l10n/common/test/scoped_default_locale.h"
#include "net/http/http_status_code.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

namespace {

struct ParamInfo final {
  const char* country;
  const char* region;
} constexpr kTests[] = {
    {.country = "US", .region = "AL"}, {.country = "US", .region = "AK"},
    {.country = "US", .region = "AZ"}, {.country = "US", .region = "AR"},
    {.country = "US", .region = "CA"}, {.country = "US", .region = "CO"},
    {.country = "US", .region = "CT"}, {.country = "US", .region = "DE"},
    {.country = "US", .region = "FL"}, {.country = "US", .region = "GA"},
    {.country = "US", .region = "HI"}, {.country = "US", .region = "ID"},
    {.country = "US", .region = "IL"}, {.country = "US", .region = "IN"},
    {.country = "US", .region = "IA"}, {.country = "US", .region = "KS"},
    {.country = "US", .region = "KY"}, {.country = "US", .region = "LA"},
    {.country = "US", .region = "ME"}, {.country = "US", .region = "MD"},
    {.country = "US", .region = "MA"}, {.country = "US", .region = "MI"},
    {.country = "US", .region = "MN"}, {.country = "US", .region = "MS"},
    {.country = "US", .region = "MO"}, {.country = "US", .region = "MT"},
    {.country = "US", .region = "NE"}, {.country = "US", .region = "NV"},
    {.country = "US", .region = "NH"}, {.country = "US", .region = "NJ"},
    {.country = "US", .region = "NM"}, {.country = "US", .region = "NY"},
    {.country = "US", .region = "NC"}, {.country = "US", .region = "ND"},
    {.country = "US", .region = "OH"}, {.country = "US", .region = "OK"},
    {.country = "US", .region = "OR"}, {.country = "US", .region = "PA"},
    {.country = "US", .region = "RI"}, {.country = "US", .region = "SC"},
    {.country = "US", .region = "SD"}, {.country = "US", .region = "TN"},
    {.country = "US", .region = "TX"}, {.country = "US", .region = "UT"},
    {.country = "US", .region = "VT"}, {.country = "US", .region = "VA"},
    {.country = "US", .region = "WA"}, {.country = "US", .region = "WV"},
    {.country = "US", .region = "WI"}, {.country = "US", .region = "WY"},
    {.country = "CA", .region = "AB"}, {.country = "CA", .region = "BC"},
    {.country = "CA", .region = "MB"}, {.country = "CA", .region = "NB"},
    {.country = "CA", .region = "NS"}, {.country = "CA", .region = "ON"},
    {.country = "CA", .region = "QC"}, {.country = "CA", .region = "SK"}};

}  // namespace

class BraveAdsSubdivisionTargetingExclusionRuleTest
    : public UnitTestBase,
      public ::testing::WithParamInterface<ParamInfo> {
 protected:
  void SetUp() override {
    UnitTestBase::SetUp();

    scoped_default_locale_ =
        std::make_unique<brave_l10n::test::ScopedDefaultLocale>(
            base::StrCat({"en", "_", GetParam().country}));

    subdivision_targeting_ = std::make_unique<SubdivisionTargeting>();
    subdivision_ = std::make_unique<Subdivision>();
    subdivision_->AddObserver(subdivision_targeting_.get());
    exclusion_rule_ = std::make_unique<SubdivisionTargetingExclusionRule>(
        *subdivision_targeting_);
  }

  static std::string BuildSubdivisionForTestParam() {
    return BuildSubdivisionForTesting(GetParam().country, GetParam().region);
  }

  static std::string BuildOtherSubdivisionForTestParam() {
    const char* region = "";

    if (GetParam().country ==
        std::string_view("US") /*United States of America*/) {
      if (GetParam().region == std::string_view("FL") /*Florida*/) {
        region = "CA";  // Alabama
      } else {
        region = "FL";  // Florida
      }
    } else if (GetParam().country == std::string_view("CA") /*Canada*/) {
      if (GetParam().region == std::string_view("QC") /*Quebec*/) {
        region = "AB";  // Alberta
      } else {
        region = "QC";  // Quebec
      }
    }

    return BuildSubdivisionForTesting(GetParam().country, region);
  }

  void MockUrlResponseForTestParam() {
    const URLResponseMap url_responses = {
        {BuildSubdivisionUrlPath(),
         {{net::HTTP_OK, BuildSubdivisionUrlResponseBodyForTesting(
                             GetParam().country, GetParam().region)}}}};
    MockUrlResponses(ads_client_mock_, url_responses);
  }

  std::unique_ptr<brave_l10n::test::ScopedDefaultLocale> scoped_default_locale_;

  std::unique_ptr<SubdivisionTargeting> subdivision_targeting_;
  std::unique_ptr<Subdivision> subdivision_;
  std::unique_ptr<SubdivisionTargetingExclusionRule> exclusion_rule_;
};

TEST_P(BraveAdsSubdivisionTargetingExclusionRuleTest,
       ShouldExcludeIfSubdivisionTargetingIsNotAllowedForGeoTargetWithRegion) {
  // Arrange
  CreativeAdInfo creative_ad;
  creative_ad.creative_set_id = kCreativeSetId;
  creative_ad.geo_targets = {BuildSubdivisionForTestParam()};

  // Act & Assert
  EXPECT_FALSE(exclusion_rule_->ShouldInclude(creative_ad).has_value());
}

TEST_P(
    BraveAdsSubdivisionTargetingExclusionRuleTest,
    ShouldIncludeIfSubdivisionTargetingIsNotAllowedForGeoTargetWithNoRegion) {
  // Arrange
  SetBooleanPrefValue(prefs::kShouldAllowSubdivisionTargeting, false);

  CreativeAdInfo creative_ad;
  creative_ad.creative_set_id = kCreativeSetId;
  creative_ad.geo_targets = {GetParam().country};

  // Act & Assert
  EXPECT_TRUE(exclusion_rule_->ShouldInclude(creative_ad).has_value());
}

TEST_P(BraveAdsSubdivisionTargetingExclusionRuleTest,
       ShouldIncludeIfSubdivisionTargetingIsSupportedAndAutoDetected) {
  // Arrange
  SetBooleanPrefValue(prefs::kShouldAllowSubdivisionTargeting, true);

  MockUrlResponseForTestParam();

  NotifyDidInitializeAds();

  CreativeAdInfo creative_ad;
  creative_ad.creative_set_id = kCreativeSetId;
  creative_ad.geo_targets = {BuildSubdivisionForTestParam()};

  // Act & Assert
  EXPECT_TRUE(exclusion_rule_->ShouldInclude(creative_ad).has_value());
}

TEST_P(
    BraveAdsSubdivisionTargetingExclusionRuleTest,
    ShouldIncludeIfSubdivisionTargetingIsSupportedAndAutoDetectedForMultipleGeoTargets) {
  // Arrange
  SetBooleanPrefValue(prefs::kShouldAllowSubdivisionTargeting, true);

  MockUrlResponseForTestParam();

  NotifyDidInitializeAds();

  CreativeAdInfo creative_ad;
  creative_ad.creative_set_id = kCreativeSetId;
  creative_ad.geo_targets = {BuildSubdivisionForTestParam(),
                             BuildOtherSubdivisionForTestParam()};

  // Act & Assert
  EXPECT_TRUE(exclusion_rule_->ShouldInclude(creative_ad).has_value());
}

TEST_P(
    BraveAdsSubdivisionTargetingExclusionRuleTest,
    ShouldIncludeIfSubdivisionTargetingIsSupportedAndAutoDetectedForGeoTargetWithNoRegion) {
  // Arrange
  SetBooleanPrefValue(prefs::kShouldAllowSubdivisionTargeting, true);

  MockUrlResponseForTestParam();

  NotifyDidInitializeAds();

  CreativeAdInfo creative_ad;
  creative_ad.creative_set_id = kCreativeSetId;
  creative_ad.geo_targets = {GetParam().country};

  // Act & Assert
  EXPECT_TRUE(exclusion_rule_->ShouldInclude(creative_ad).has_value());
}

TEST_P(
    BraveAdsSubdivisionTargetingExclusionRuleTest,
    ShouldIncludeIfSubdivisionTargetingIsSupportedAndSubdivisionWasManuallySelected) {
  // Arrange
  SetBooleanPrefValue(prefs::kShouldAllowSubdivisionTargeting, true);

  SetStringPrefValue(prefs::kSubdivisionTargetingSubdivision,
                     BuildSubdivisionForTestParam());

  MockUrlResponseForTestParam();

  NotifyDidInitializeAds();

  CreativeAdInfo creative_ad;
  creative_ad.creative_set_id = kCreativeSetId;
  creative_ad.geo_targets = {BuildSubdivisionForTestParam()};

  // Act & Assert
  EXPECT_TRUE(exclusion_rule_->ShouldInclude(creative_ad).has_value());
}

TEST_P(
    BraveAdsSubdivisionTargetingExclusionRuleTest,
    ShouldIncludeIfSubdivisionTargetingIsSupportedAndSubdivisionWasManuallySelectedForMultipleGeoTargets) {
  // Arrange
  SetBooleanPrefValue(prefs::kShouldAllowSubdivisionTargeting, true);

  SetStringPrefValue(prefs::kSubdivisionTargetingSubdivision,
                     BuildSubdivisionForTestParam());

  MockUrlResponseForTestParam();

  NotifyDidInitializeAds();

  CreativeAdInfo creative_ad;
  creative_ad.creative_set_id = kCreativeSetId;
  creative_ad.geo_targets = {BuildSubdivisionForTestParam(),
                             BuildOtherSubdivisionForTestParam()};

  // Act & Assert
  EXPECT_TRUE(exclusion_rule_->ShouldInclude(creative_ad).has_value());
}

TEST_P(
    BraveAdsSubdivisionTargetingExclusionRuleTest,
    ShouldIncludeIfSubdivisionTargetingIsSupportedAndManuallySelectedForGeoTargetWithNoRegion) {
  // Arrange
  SetBooleanPrefValue(prefs::kShouldAllowSubdivisionTargeting, true);

  SetStringPrefValue(prefs::kSubdivisionTargetingSubdivision,
                     BuildSubdivisionForTestParam());

  MockUrlResponseForTestParam();

  NotifyDidInitializeAds();

  CreativeAdInfo creative_ad;
  creative_ad.creative_set_id = kCreativeSetId;
  creative_ad.geo_targets = {GetParam().country};

  // Act & Assert
  EXPECT_TRUE(exclusion_rule_->ShouldInclude(creative_ad).has_value());
}

TEST_P(BraveAdsSubdivisionTargetingExclusionRuleTest,
       ShouldExcludeIfSubdivisionTargetingIsSupportedForUnsupportedGeoTarget) {
  // Arrange
  SetBooleanPrefValue(prefs::kShouldAllowSubdivisionTargeting, true);

  MockUrlResponseForTestParam();

  NotifyDidInitializeAds();

  CreativeAdInfo creative_ad;
  creative_ad.creative_set_id = kCreativeSetId;
  creative_ad.geo_targets = {BuildSubdivisionForTesting(
      /*country_code=*/"US", /*subdivision_code=*/"XX")};

  // Assert
  EXPECT_FALSE(exclusion_rule_->ShouldInclude(creative_ad).has_value());
}

TEST_P(
    BraveAdsSubdivisionTargetingExclusionRuleTest,
    ShouldExcludeIfSubdivisionTargetingIsNotSupportedForSubdivisionGeoTarget) {
  // Arrange
  SetBooleanPrefValue(prefs::kShouldAllowSubdivisionTargeting, true);

  MockUrlResponseForTestParam();

  NotifyDidInitializeAds();

  CreativeAdInfo creative_ad;
  creative_ad.creative_set_id = kCreativeSetId;
  creative_ad.geo_targets = {BuildSubdivisionForTesting(
      /*country_code=*/"GB", /*subdivision_code=*/"DEV")};

  // Act & Assert
  EXPECT_FALSE(exclusion_rule_->ShouldInclude(creative_ad).has_value());
}

TEST_P(
    BraveAdsSubdivisionTargetingExclusionRuleTest,
    ShouldIncludeIfSubdivisionTargetingIsNotSupportedForNonSubdivisionGeoTarget) {
  // Arrange
  SetBooleanPrefValue(prefs::kShouldAllowSubdivisionTargeting, true);

  const URLResponseMap url_responses = {
      {BuildSubdivisionUrlPath(),
       {{net::HTTP_OK,
         BuildSubdivisionUrlResponseBodyForTesting(
             /*country_code=*/"XX", /*subdivision_code=*/"NO REGION")}}}};
  MockUrlResponses(ads_client_mock_, url_responses);

  NotifyDidInitializeAds();

  CreativeAdInfo creative_ad;
  creative_ad.creative_set_id = kCreativeSetId;
  creative_ad.geo_targets = {"XX"};

  // Act & Assert
  EXPECT_TRUE(exclusion_rule_->ShouldInclude(creative_ad).has_value());
}

TEST_P(BraveAdsSubdivisionTargetingExclusionRuleTest,
       ShouldExcludeIfSubdivisionTargetingIsDisabledForSubdivisionGeoTarget) {
  // Arrange
  SetBooleanPrefValue(prefs::kShouldAllowSubdivisionTargeting, true);

  SetStringPrefValue(prefs::kSubdivisionTargetingSubdivision, "DISABLED");

  MockUrlResponseForTestParam();

  NotifyDidInitializeAds();

  CreativeAdInfo creative_ad;
  creative_ad.creative_set_id = kCreativeSetId;
  creative_ad.geo_targets = {BuildSubdivisionForTestParam()};

  // Act & Assert
  EXPECT_FALSE(exclusion_rule_->ShouldInclude(creative_ad).has_value());
}

TEST_P(
    BraveAdsSubdivisionTargetingExclusionRuleTest,
    ShouldIncludeIfSubdivisionTargetingIsDisabledForNonSubdivisionGeoTarget) {
  // Arrange
  SetBooleanPrefValue(prefs::kShouldAllowSubdivisionTargeting, true);

  SetStringPrefValue(prefs::kSubdivisionTargetingSubdivision, "DISABLED");

  MockUrlResponseForTestParam();

  NotifyDidInitializeAds();

  CreativeAdInfo creative_ad;
  creative_ad.creative_set_id = kCreativeSetId;
  creative_ad.geo_targets = {"XX"};

  // Act & Assert
  EXPECT_TRUE(exclusion_rule_->ShouldInclude(creative_ad).has_value());
}

std::string TestParamToString(::testing::TestParamInfo<ParamInfo> test_param) {
  return base::ReplaceStringPlaceholders(
      "Country$1_Region$2", {test_param.param.country, test_param.param.region},
      nullptr);
}

INSTANTIATE_TEST_SUITE_P(,
                         BraveAdsSubdivisionTargetingExclusionRuleTest,
                         ::testing::ValuesIn(kTests),
                         TestParamToString);

}  // namespace brave_ads
