/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/ads/serving/eligible_ads/exclusion_rules/subdivision_targeting_exclusion_rule.h"

#include <memory>

#include "base/strings/strcat.h"
#include "base/strings/string_util.h"
#include "brave/components/brave_ads/common/pref_names.h"
#include "brave/components/brave_ads/core/internal/ads/ad_unittest_constants.h"
#include "brave/components/brave_ads/core/internal/common/locale/subdivision_code_unittest_util.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_base.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_mock_util.h"
#include "brave/components/brave_ads/core/internal/geographic/subdivision/subdivision_targeting.h"
#include "brave/components/brave_ads/core/internal/geographic/subdivision/subdivision_targeting_unittest_util.h"
#include "brave/components/l10n/common/test/scoped_default_locale.h"
#include "net/http/http_status_code.h"

// npm run test -- brave_unit_tests --filter=BatAds*

namespace brave_ads {

namespace {

struct TestParam final {
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

class BatAdsSubdivisionTargetingExclusionRuleTest
    : public UnitTestBase,
      public ::testing::WithParamInterface<TestParam> {
 protected:
  void SetUp() override {
    UnitTestBase::SetUp();

    scoped_default_locale_ =
        std::make_unique<brave_l10n::test::ScopedDefaultLocale>(
            base::StrCat({"en", "_", GetParam().country}));

    subdivision_targeting_ =
        std::make_unique<geographic::SubdivisionTargeting>();
    exclusion_rule_ = std::make_unique<SubdivisionTargetingExclusionRule>(
        *subdivision_targeting_);
  }

  static std::string BuildSubdivisionCode() {
    return locale::BuildSubdivisionCode(GetParam().country, GetParam().region);
  }

  static std::string BuildOtherSubdivisionCode() {
    const char* region = "";

    if (GetParam().country ==
        base::StringPiece("US") /*United States of America*/) {
      region = "FL";  // Florida

      if (GetParam().region == region) {
        region = "AL";  // Alabama
      }
    } else if (GetParam().country == base::StringPiece("CA") /*Canada*/) {
      region = "QC";  // Quebec

      if (GetParam().region == region) {
        region = "AB";  // Alberta
      }
    }

    return locale::BuildSubdivisionCode(GetParam().country, region);
  }

  std::unique_ptr<brave_l10n::test::ScopedDefaultLocale> scoped_default_locale_;

  std::unique_ptr<geographic::SubdivisionTargeting> subdivision_targeting_;
  std::unique_ptr<SubdivisionTargetingExclusionRule> exclusion_rule_;
};

TEST_P(BatAdsSubdivisionTargetingExclusionRuleTest,
       DoNotAllowAdIfSubdivisionTargetingIsNotAllowedForGeoTargetWithRegion) {
  // Arrange
  CreativeAdInfo creative_ad;
  creative_ad.creative_set_id = kCreativeSetId;
  creative_ad.geo_targets = {BuildSubdivisionCode()};

  // Act

  // Assert
  EXPECT_TRUE(exclusion_rule_->ShouldExclude(creative_ad));
}

TEST_P(BatAdsSubdivisionTargetingExclusionRuleTest,
       AllowAdIfSubdivisionTargetingIsNotAllowedForGeoTargetWithNoRegion) {
  // Arrange
  ads_client_mock_->SetBooleanPref(prefs::kShouldAllowSubdivisionTargeting,
                                   false);

  CreativeAdInfo creative_ad;
  creative_ad.creative_set_id = kCreativeSetId;
  creative_ad.geo_targets = {GetParam().country};

  // Act

  // Assert
  EXPECT_FALSE(exclusion_rule_->ShouldExclude(creative_ad));
}

TEST_P(BatAdsSubdivisionTargetingExclusionRuleTest,
       AllowAdIfSubdivisionTargetingIsSupportedAndAutoDetected) {
  // Arrange
  ads_client_mock_->SetBooleanPref(prefs::kShouldAllowSubdivisionTargeting,
                                   true);

  const URLResponseMap url_responses =
      geographic::BuildValidSubdivisionTargetingUrlResponses(
          net::HTTP_OK, GetParam().country, GetParam().region);
  MockUrlResponses(ads_client_mock_, url_responses);

  subdivision_targeting_->MaybeFetch();

  CreativeAdInfo creative_ad;
  creative_ad.creative_set_id = kCreativeSetId;
  creative_ad.geo_targets = {BuildSubdivisionCode()};

  // Act

  // Assert
  EXPECT_FALSE(exclusion_rule_->ShouldExclude(creative_ad));
}

TEST_P(
    BatAdsSubdivisionTargetingExclusionRuleTest,
    AllowAdIfSubdivisionTargetingIsSupportedAndAutoDetectedForMultipleGeoTargets) {
  // Arrange
  ads_client_mock_->SetBooleanPref(prefs::kShouldAllowSubdivisionTargeting,
                                   true);

  const URLResponseMap url_responses =
      geographic::BuildValidSubdivisionTargetingUrlResponses(
          net::HTTP_OK, GetParam().country, GetParam().region);
  MockUrlResponses(ads_client_mock_, url_responses);

  subdivision_targeting_->MaybeFetch();

  CreativeAdInfo creative_ad;
  creative_ad.creative_set_id = kCreativeSetId;
  creative_ad.geo_targets = {BuildSubdivisionCode(),
                             BuildOtherSubdivisionCode()};

  // Act

  // Assert
  EXPECT_FALSE(exclusion_rule_->ShouldExclude(creative_ad));
}

TEST_P(
    BatAdsSubdivisionTargetingExclusionRuleTest,
    AllowAdIfSubdivisionTargetingIsSupportedAndAutoDetectedForGeoTargetWithNoRegion) {
  // Arrange
  ads_client_mock_->SetBooleanPref(prefs::kShouldAllowSubdivisionTargeting,
                                   true);

  const URLResponseMap url_responses =
      geographic::BuildValidSubdivisionTargetingUrlResponses(
          net::HTTP_OK, GetParam().country, GetParam().region);
  MockUrlResponses(ads_client_mock_, url_responses);

  subdivision_targeting_->MaybeFetch();

  CreativeAdInfo creative_ad;
  creative_ad.creative_set_id = kCreativeSetId;
  creative_ad.geo_targets = {GetParam().country};

  // Act

  // Assert
  EXPECT_FALSE(exclusion_rule_->ShouldExclude(creative_ad));
}

TEST_P(
    BatAdsSubdivisionTargetingExclusionRuleTest,
    AllowAdIfSubdivisionTargetingIsSupportedAndSubdivisionWasManuallySelected) {
  // Arrange
  ads_client_mock_->SetBooleanPref(prefs::kShouldAllowSubdivisionTargeting,
                                   true);

  ads_client_mock_->SetStringPref(prefs::kSubdivisionTargetingCode,
                                  BuildSubdivisionCode());

  const URLResponseMap url_responses =
      geographic::BuildValidSubdivisionTargetingUrlResponses(
          net::HTTP_OK, GetParam().country, GetParam().region);
  MockUrlResponses(ads_client_mock_, url_responses);

  subdivision_targeting_->MaybeFetch();

  CreativeAdInfo creative_ad;
  creative_ad.creative_set_id = kCreativeSetId;
  creative_ad.geo_targets = {BuildSubdivisionCode()};

  // Act

  // Assert
  EXPECT_FALSE(exclusion_rule_->ShouldExclude(creative_ad));
}

TEST_P(
    BatAdsSubdivisionTargetingExclusionRuleTest,
    AllowAdIfSubdivisionTargetingIsSupportedAndSubdivisionWasManuallySelectedForMultipleGeoTargets) {
  // Arrange
  ads_client_mock_->SetBooleanPref(prefs::kShouldAllowSubdivisionTargeting,
                                   true);

  ads_client_mock_->SetStringPref(prefs::kSubdivisionTargetingCode,
                                  BuildSubdivisionCode());

  const URLResponseMap url_responses =
      geographic::BuildValidSubdivisionTargetingUrlResponses(
          net::HTTP_OK, GetParam().country, GetParam().region);
  MockUrlResponses(ads_client_mock_, url_responses);

  subdivision_targeting_->MaybeFetch();

  CreativeAdInfo creative_ad;
  creative_ad.creative_set_id = kCreativeSetId;
  creative_ad.geo_targets = {BuildSubdivisionCode(),
                             BuildOtherSubdivisionCode()};

  // Act

  // Assert
  EXPECT_FALSE(exclusion_rule_->ShouldExclude(creative_ad));
}

TEST_P(
    BatAdsSubdivisionTargetingExclusionRuleTest,
    AllowAdIfSubdivisionTargetingIsSupportedAndManuallySelectedForGeoTargetWithNoRegion) {
  // Arrange
  ads_client_mock_->SetBooleanPref(prefs::kShouldAllowSubdivisionTargeting,
                                   true);

  ads_client_mock_->SetStringPref(prefs::kSubdivisionTargetingCode,
                                  BuildSubdivisionCode());

  const URLResponseMap url_responses =
      geographic::BuildValidSubdivisionTargetingUrlResponses(
          net::HTTP_OK, GetParam().country, GetParam().region);
  MockUrlResponses(ads_client_mock_, url_responses);

  subdivision_targeting_->MaybeFetch();

  CreativeAdInfo creative_ad;
  creative_ad.creative_set_id = kCreativeSetId;
  creative_ad.geo_targets = {GetParam().country};

  // Act

  // Assert
  EXPECT_FALSE(exclusion_rule_->ShouldExclude(creative_ad));
}

TEST_P(BatAdsSubdivisionTargetingExclusionRuleTest,
       DoNotAllowAdIfSubdivisionTargetingIsSupportedForUnsupportedGeoTarget) {
  // Arrange
  ads_client_mock_->SetBooleanPref(prefs::kShouldAllowSubdivisionTargeting,
                                   true);

  const URLResponseMap url_responses =
      geographic::BuildValidSubdivisionTargetingUrlResponses(
          net::HTTP_OK, GetParam().country, GetParam().region);
  MockUrlResponses(ads_client_mock_, url_responses);

  subdivision_targeting_->MaybeFetch();

  CreativeAdInfo creative_ad;
  creative_ad.creative_set_id = kCreativeSetId;
  creative_ad.geo_targets = {
      locale::BuildSubdivisionCode(/*country*/ "US", /*region*/ "XX")};

  // Assert
  EXPECT_TRUE(exclusion_rule_->ShouldExclude(creative_ad));
}

TEST_P(
    BatAdsSubdivisionTargetingExclusionRuleTest,
    DoNotAllowAdIfSubdivisionTargetingIsNotSupportedForSubdivisionGeoTarget) {
  // Arrange
  ads_client_mock_->SetBooleanPref(prefs::kShouldAllowSubdivisionTargeting,
                                   true);

  const URLResponseMap url_responses =
      geographic::BuildValidSubdivisionTargetingUrlResponses(
          net::HTTP_OK, GetParam().country, GetParam().region);
  MockUrlResponses(ads_client_mock_, url_responses);

  subdivision_targeting_->MaybeFetch();

  CreativeAdInfo creative_ad;
  creative_ad.creative_set_id = kCreativeSetId;
  creative_ad.geo_targets = {
      locale::BuildSubdivisionCode(/*country*/ "GB", /*region*/ "DEV")};

  // Act

  // Assert
  EXPECT_TRUE(exclusion_rule_->ShouldExclude(creative_ad));
}

TEST_P(BatAdsSubdivisionTargetingExclusionRuleTest,
       AllowAdIfSubdivisionTargetingIsNotSupportedForNonSubdivisionGeoTarget) {
  // Arrange
  ads_client_mock_->SetBooleanPref(prefs::kShouldAllowSubdivisionTargeting,
                                   true);

  const URLResponseMap url_responses =
      geographic::BuildValidSubdivisionTargetingUrlResponses(
          net::HTTP_OK, /*country*/ "XX",
          /*region*/ "NO REGION");
  MockUrlResponses(ads_client_mock_, url_responses);

  subdivision_targeting_->MaybeFetch();

  CreativeAdInfo creative_ad;
  creative_ad.creative_set_id = kCreativeSetId;
  creative_ad.geo_targets = {"XX"};

  // Act

  // Assert
  EXPECT_FALSE(exclusion_rule_->ShouldExclude(creative_ad));
}

TEST_P(BatAdsSubdivisionTargetingExclusionRuleTest,
       DoNotAllowAdIfSubdivisionTargetingIsDisabledForSubdivisionGeoTarget) {
  // Arrange
  ads_client_mock_->SetBooleanPref(prefs::kShouldAllowSubdivisionTargeting,
                                   true);

  ads_client_mock_->SetStringPref(prefs::kSubdivisionTargetingCode, "DISABLED");

  const URLResponseMap url_responses =
      geographic::BuildValidSubdivisionTargetingUrlResponses(
          net::HTTP_OK, GetParam().country, GetParam().region);
  MockUrlResponses(ads_client_mock_, url_responses);

  subdivision_targeting_->MaybeFetch();

  CreativeAdInfo creative_ad;
  creative_ad.creative_set_id = kCreativeSetId;
  creative_ad.geo_targets = {BuildSubdivisionCode()};

  // Act

  // Assert
  EXPECT_TRUE(exclusion_rule_->ShouldExclude(creative_ad));
}

TEST_P(BatAdsSubdivisionTargetingExclusionRuleTest,
       AllowAdIfSubdivisionTargetingIsDisabledForNonSubdivisionGeoTarget) {
  // Arrange
  ads_client_mock_->SetBooleanPref(prefs::kShouldAllowSubdivisionTargeting,
                                   true);

  ads_client_mock_->SetStringPref(prefs::kSubdivisionTargetingCode, "DISABLED");

  const URLResponseMap url_responses =
      geographic::BuildValidSubdivisionTargetingUrlResponses(
          net::HTTP_OK, GetParam().country, GetParam().region);
  MockUrlResponses(ads_client_mock_, url_responses);

  subdivision_targeting_->MaybeFetch();

  CreativeAdInfo creative_ad;
  creative_ad.creative_set_id = kCreativeSetId;
  creative_ad.geo_targets = {"XX"};

  // Act

  // Assert
  EXPECT_FALSE(exclusion_rule_->ShouldExclude(creative_ad));
}

std::string TestParamToString(::testing::TestParamInfo<TestParam> test_param) {
  return base::ReplaceStringPlaceholders(
      "Country$1_Region$2", {test_param.param.country, test_param.param.region},
      nullptr);
}

INSTANTIATE_TEST_SUITE_P(,
                         BatAdsSubdivisionTargetingExclusionRuleTest,
                         ::testing::ValuesIn(kTests),
                         TestParamToString);

}  // namespace brave_ads
