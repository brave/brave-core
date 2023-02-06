/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/ads/serving/eligible_ads/exclusion_rules/subdivision_targeting_exclusion_rule.h"

#include <memory>

#include "base/strings/strcat.h"
#include "bat/ads/internal/common/unittest/unittest_base.h"
#include "bat/ads/internal/common/unittest/unittest_mock_util.h"
#include "bat/ads/internal/geographic/subdivision/subdivision_targeting.h"
#include "brave/components/brave_ads/common/pref_names.h"
#include "brave/components/l10n/common/test/scoped_default_locale.h"
#include "net/http/http_status_code.h"

// npm run test -- brave_unit_tests --filter=BatAds*

namespace ads {

namespace {

constexpr char kCreativeSetId[] = "654f10df-fbc4-4a92-8d43-2edf73734a60";

struct SubdivisionTargetingExclusionRuleTestParam {
  const char* country;
  const char* region;
};

}  // namespace

class BatAdsSubdivisionTargetingExclusionRuleTest
    : public UnitTestBase,
      public ::testing::WithParamInterface<
          SubdivisionTargetingExclusionRuleTestParam> {
 protected:
  void SetUp() override {
    UnitTestBase::SetUp();

    scoped_default_locale_ =
        std::make_unique<brave_l10n::test::ScopedDefaultLocale>(
            base::StrCat({"en", "_", GetParam().country}));

    subdivision_targeting_ =
        std::make_unique<geographic::SubdivisionTargeting>();
    exclusion_rule_ = std::make_unique<SubdivisionTargetingExclusionRule>(
        subdivision_targeting_.get());
  }

  static std::string GetCountryParam() { return GetParam().country; }

  static std::string GetGeoTargetResponseParam() {
    return base::StrCat({R"({"country":")", GetParam().country,
                         R"(", "region":")", GetParam().region, R"("})"});
  }

  static std::string GetSubdivisionParam() {
    return base::StrCat({GetParam().country, "-", GetParam().region});
  }

  static std::string GetAdditionalSubdivisionParam() {
    const char* additional_region = "";
    if (GetParam().country == base::StringPiece("US")) {
      additional_region = "CA";
      if (GetParam().region == base::StringPiece("CA")) {
        additional_region = "AL";
      }
    } else if (GetParam().country == base::StringPiece("CA")) {
      additional_region = "QC";
      if (GetParam().region == base::StringPiece("QC")) {
        additional_region = "AB";
      }
    }
    return base::StrCat({GetParam().country, "-", additional_region});
  }

  static std::string GetUnsupportedSubdivisionParam() {
    return base::StrCat({GetParam().country, "-XX"});
  }

  std::unique_ptr<brave_l10n::test::ScopedDefaultLocale> scoped_default_locale_;

  std::unique_ptr<geographic::SubdivisionTargeting> subdivision_targeting_;
  std::unique_ptr<SubdivisionTargetingExclusionRule> exclusion_rule_;
};

TEST_P(BatAdsSubdivisionTargetingExclusionRuleTest,
       AllowAdIfSubdivisionTargetingIsSupportedAndAutoDetected) {
  // Arrange
  const URLResponseMap url_responses = {
      {"/v1/getstate", {{net::HTTP_OK, GetGeoTargetResponseParam()}}}};
  MockUrlResponses(ads_client_mock_, url_responses);

  subdivision_targeting_->MaybeFetch();

  CreativeAdInfo creative_ad;
  creative_ad.creative_set_id = kCreativeSetId;
  creative_ad.geo_targets = {GetSubdivisionParam()};

  // Act
  const bool should_exclude = exclusion_rule_->ShouldExclude(creative_ad);

  // Assert
  EXPECT_FALSE(should_exclude);
}

TEST_P(BatAdsSubdivisionTargetingExclusionRuleTest,
       AllowAdIfSubdivisionTargetingIsSupportedForMultipleGeoTargets) {
  // Arrange
  const URLResponseMap url_responses = {
      {"/v1/getstate", {{net::HTTP_OK, GetGeoTargetResponseParam()}}}};
  MockUrlResponses(ads_client_mock_, url_responses);

  subdivision_targeting_->MaybeFetch();

  CreativeAdInfo creative_ad;
  creative_ad.creative_set_id = kCreativeSetId;
  creative_ad.geo_targets = {GetSubdivisionParam(),
                             GetAdditionalSubdivisionParam()};

  // Act
  const bool should_exclude = exclusion_rule_->ShouldExclude(creative_ad);

  // Assert
  EXPECT_FALSE(should_exclude);
}

TEST_P(
    BatAdsSubdivisionTargetingExclusionRuleTest,
    AllowAdIfSubdivisionTargetingIsSupportedAndAutoDetectedForNonSubdivisionGeoTarget) {  // NOLINT
  // Arrange
  const URLResponseMap url_responses = {
      {"/v1/getstate", {{net::HTTP_OK, GetGeoTargetResponseParam()}}}};
  MockUrlResponses(ads_client_mock_, url_responses);

  subdivision_targeting_->MaybeFetch();

  CreativeAdInfo creative_ad;
  creative_ad.creative_set_id = kCreativeSetId;
  creative_ad.geo_targets = {GetCountryParam()};

  // Act
  const bool should_exclude = exclusion_rule_->ShouldExclude(creative_ad);

  // Assert
  EXPECT_FALSE(should_exclude);
}

TEST_P(BatAdsSubdivisionTargetingExclusionRuleTest,
       AllowAdIfSubdivisionTargetingIsSupportedAndManuallySelected) {
  // Arrange
  const URLResponseMap url_responses = {
      {"/v1/getstate", {{net::HTTP_OK, GetGeoTargetResponseParam()}}}};
  MockUrlResponses(ads_client_mock_, url_responses);

  subdivision_targeting_->MaybeFetch();

  CreativeAdInfo creative_ad;
  creative_ad.creative_set_id = kCreativeSetId;
  creative_ad.geo_targets = {GetSubdivisionParam()};

  // Act
  const bool should_exclude = exclusion_rule_->ShouldExclude(creative_ad);

  // Assert
  EXPECT_FALSE(should_exclude);
}

TEST_P(
    BatAdsSubdivisionTargetingExclusionRuleTest,
    AllowAdIfSubdivisionTargetingIsSupportedAndManuallySelectedForNonSubdivisionGeoTarget) {  // NOLINT
  // Arrange
  const URLResponseMap url_responses = {
      {"/v1/getstate", {{net::HTTP_OK, GetGeoTargetResponseParam()}}}};
  MockUrlResponses(ads_client_mock_, url_responses);

  subdivision_targeting_->MaybeFetch();

  CreativeAdInfo creative_ad;
  creative_ad.creative_set_id = kCreativeSetId;
  creative_ad.geo_targets = {GetCountryParam()};

  // Act
  const bool should_exclude = exclusion_rule_->ShouldExclude(creative_ad);

  // Assert
  EXPECT_FALSE(should_exclude);
}

TEST_P(BatAdsSubdivisionTargetingExclusionRuleTest,
       DoNotAllowAdIfSubdivisionTargetingIsNotSupportedOrNotInitialized) {
  // Arrange
  CreativeAdInfo creative_ad;
  creative_ad.creative_set_id = kCreativeSetId;
  creative_ad.geo_targets = {GetSubdivisionParam()};

  // Act
  const bool should_exclude = exclusion_rule_->ShouldExclude(creative_ad);

  // Assert
  EXPECT_TRUE(should_exclude);
}

TEST_P(BatAdsSubdivisionTargetingExclusionRuleTest,
       DoNotAllowAdIfSubdivisionTargetingIsSupportedForUnsupportedGeoTarget) {
  // Arrange
  const URLResponseMap url_responses = {
      {"/v1/getstate", {{net::HTTP_OK, GetGeoTargetResponseParam()}}}};
  MockUrlResponses(ads_client_mock_, url_responses);

  subdivision_targeting_->MaybeFetch();

  CreativeAdInfo creative_ad;
  creative_ad.creative_set_id = kCreativeSetId;
  creative_ad.geo_targets = {GetUnsupportedSubdivisionParam()};

  // Act
  const bool should_exclude = exclusion_rule_->ShouldExclude(creative_ad);

  // Assert
  EXPECT_TRUE(should_exclude);
}

TEST_P(
    BatAdsSubdivisionTargetingExclusionRuleTest,
    DoNotAllowAdIfSubdivisionTargetingIsNotSupportedForSubdivisionGeoTarget) {
  // Arrange
  const URLResponseMap url_responses = {
      {"/v1/getstate", {{net::HTTP_OK, GetGeoTargetResponseParam()}}}};
  MockUrlResponses(ads_client_mock_, url_responses);

  subdivision_targeting_->MaybeFetch();

  CreativeAdInfo creative_ad;
  creative_ad.creative_set_id = kCreativeSetId;
  creative_ad.geo_targets = {"GB-DEV"};

  // Act
  const bool should_exclude = exclusion_rule_->ShouldExclude(creative_ad);

  // Assert
  EXPECT_TRUE(should_exclude);
}

TEST_P(BatAdsSubdivisionTargetingExclusionRuleTest,
       AllowAdIfSubdivisionTargetingIsNotSupportedForNonSubdivisionGeoTarget) {
  // Arrange
  const URLResponseMap url_responses = {{"/v1/getstate", {{net::HTTP_OK, R"(
            {"country":"XX", "region":"NO REGION"}
          )"}}}};
  MockUrlResponses(ads_client_mock_, url_responses);

  subdivision_targeting_->MaybeFetch();

  CreativeAdInfo creative_ad;
  creative_ad.creative_set_id = kCreativeSetId;
  creative_ad.geo_targets = {"XX"};

  // Act
  const bool should_exclude = exclusion_rule_->ShouldExclude(creative_ad);

  // Assert
  EXPECT_FALSE(should_exclude);
}

TEST_P(BatAdsSubdivisionTargetingExclusionRuleTest,
       DoNotAllowAdIfSubdivisionTargetingIsDisabledForSubdivisionGeoTarget) {
  // Arrange
  AdsClientHelper::GetInstance()->SetStringPref(
      prefs::kSubdivisionTargetingCode, "DISABLED");

  const URLResponseMap url_responses = {
      {"/v1/getstate", {{net::HTTP_OK, GetGeoTargetResponseParam()}}}};
  MockUrlResponses(ads_client_mock_, url_responses);

  subdivision_targeting_->MaybeFetch();

  CreativeAdInfo creative_ad;
  creative_ad.creative_set_id = kCreativeSetId;
  creative_ad.geo_targets = {GetSubdivisionParam()};

  // Act
  const bool should_exclude = exclusion_rule_->ShouldExclude(creative_ad);

  // Assert
  EXPECT_TRUE(should_exclude);
}

TEST_P(BatAdsSubdivisionTargetingExclusionRuleTest,
       AllowAdIfSubdivisionTargetingIsDisabledForNonSubdivisionGeoTarget) {
  // Arrange
  AdsClientHelper::GetInstance()->SetStringPref(
      prefs::kSubdivisionTargetingCode, "DISABLED");

  const URLResponseMap url_responses = {
      {"/v1/getstate", {{net::HTTP_OK, GetGeoTargetResponseParam()}}}};
  MockUrlResponses(ads_client_mock_, url_responses);

  subdivision_targeting_->MaybeFetch();

  CreativeAdInfo creative_ad;
  creative_ad.creative_set_id = kCreativeSetId;
  creative_ad.geo_targets = {GetCountryParam()};

  // Act
  const bool should_exclude = exclusion_rule_->ShouldExclude(creative_ad);

  // Assert
  EXPECT_FALSE(should_exclude);
}

constexpr SubdivisionTargetingExclusionRuleTestParam kTests[] = {
    {
        "US",  // country
        "AL",  // region
    },
    {
        "US",  // country
        "AK",  // region
    },
    {
        "US",  // country
        "AZ",  // region
    },
    {
        "US",  // country
        "AR",  // region
    },
    {
        "US",  // country
        "CA",  // region
    },
    {
        "US",  // country
        "CO",  // region
    },
    {
        "US",  // country
        "CT",  // region
    },
    {
        "US",  // country
        "DE",  // region
    },
    {
        "US",  // country
        "FL",  // region
    },
    {
        "US",  // country
        "GA",  // region
    },
    {
        "US",  // country
        "HI",  // region
    },
    {
        "US",  // country
        "ID",  // region
    },
    {
        "US",  // country
        "IL",  // region
    },
    {
        "US",  // country
        "IN",  // region
    },
    {
        "US",  // country
        "IA",  // region
    },
    {
        "US",  // country
        "KS",  // region
    },
    {
        "US",  // country
        "KY",  // region
    },
    {
        "US",  // country
        "LA",  // region
    },
    {
        "US",  // country
        "ME",  // region
    },
    {
        "US",  // country
        "MD",  // region
    },
    {
        "US",  // country
        "MA",  // region
    },
    {
        "US",  // country
        "MI",  // region
    },
    {
        "US",  // country
        "MN",  // region
    },
    {
        "US",  // country
        "MS",  // region
    },
    {
        "US",  // country
        "MO",  // region
    },
    {
        "US",  // country
        "MT",  // region
    },
    {
        "US",  // country
        "NE",  // region
    },
    {
        "US",  // country
        "NV",  // region
    },
    {
        "US",  // country
        "NH",  // region
    },
    {
        "US",  // country
        "NJ",  // region
    },
    {
        "US",  // country
        "NM",  // region
    },
    {
        "US",  // country
        "NY",  // region
    },
    {
        "US",  // country
        "NC",  // region
    },
    {
        "US",  // country
        "ND",  // region
    },
    {
        "US",  // country
        "OH",  // region
    },
    {
        "US",  // country
        "OK",  // region
    },
    {
        "US",  // country
        "OR",  // region
    },
    {
        "US",  // country
        "PA",  // region
    },
    {
        "US",  // country
        "RI",  // region
    },
    {
        "US",  // country
        "SC",  // region
    },
    {
        "US",  // country
        "SD",  // region
    },
    {
        "US",  // country
        "TN",  // region
    },
    {
        "US",  // country
        "TX",  // region
    },
    {
        "US",  // country
        "UT",  // region
    },
    {
        "US",  // country
        "VT",  // region
    },
    {
        "US",  // country
        "VA",  // region
    },
    {
        "US",  // country
        "WA",  // region
    },
    {
        "US",  // country
        "WV",  // region
    },
    {
        "US",  // country
        "WI",  // region
    },
    {
        "US",  // country
        "WY",  // region
    },
    {
        "CA",  // country
        "AB",  // region
    },
    {
        "CA",  // country
        "BC",  // region
    },
    {
        "CA",  // country
        "MB",  // region
    },
    {
        "CA",  // country
        "NB",  // region
    },
    {
        "CA",  // country
        "NS",  // region
    },
    {
        "CA",  // country
        "ON",  // region
    },
    {
        "CA",  // country
        "QC",  // region
    },
    {
        "CA",  // country
        "SK",  // region
    },
};

INSTANTIATE_TEST_SUITE_P(,
                         BatAdsSubdivisionTargetingExclusionRuleTest,
                         ::testing::ValuesIn(kTests));

}  // namespace ads
