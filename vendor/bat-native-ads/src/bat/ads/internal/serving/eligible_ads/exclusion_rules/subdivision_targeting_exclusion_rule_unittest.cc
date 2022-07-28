/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/serving/eligible_ads/exclusion_rules/subdivision_targeting_exclusion_rule.h"

#include <memory>

#include "bat/ads/internal/base/net/http/http_status_code.h"
#include "bat/ads/internal/base/unittest/unittest_base.h"
#include "bat/ads/internal/base/unittest/unittest_mock_util.h"
#include "bat/ads/internal/base/unittest/unittest_url_response_aliases.h"
#include "bat/ads/internal/geographic/subdivision/subdivision_targeting.h"
#include "bat/ads/pref_names.h"

// npm run test -- brave_unit_tests --filter=BatAds*

namespace ads {

namespace {
constexpr char kCreativeSetId[] = "654f10df-fbc4-4a92-8d43-2edf73734a60";
}  // namespace

class BatAdsSubdivisionTargetingExclusionRuleTest : public UnitTestBase {
 protected:
  BatAdsSubdivisionTargetingExclusionRuleTest() = default;

  ~BatAdsSubdivisionTargetingExclusionRuleTest() override = default;

  void SetUp() override {
    UnitTestBase::SetUp();

    subdivision_targeting_ =
        std::make_unique<geographic::SubdivisionTargeting>();
    exclusion_rule_ = std::make_unique<SubdivisionTargetingExclusionRule>(
        subdivision_targeting_.get());
  }

  std::unique_ptr<geographic::SubdivisionTargeting> subdivision_targeting_;
  std::unique_ptr<SubdivisionTargetingExclusionRule> exclusion_rule_;
};

TEST_F(BatAdsSubdivisionTargetingExclusionRuleTest,
       AllowAdIfSubdivisionTargetingIsSupportedAndAutoDetected) {
  // Arrange
  const URLEndpoints endpoints = {{R"(/v1/getstate)", {{net::HTTP_OK, R"(
            {"country":"US", "region":"FL"}
          )"}}}};
  MockUrlRequest(ads_client_mock_, endpoints);

  subdivision_targeting_->MaybeFetch();

  CreativeAdInfo creative_ad;
  creative_ad.creative_set_id = kCreativeSetId;
  creative_ad.geo_targets = {"US-FL"};

  // Act
  const bool should_exclude = exclusion_rule_->ShouldExclude(creative_ad);

  // Assert
  EXPECT_FALSE(should_exclude);
}

TEST_F(BatAdsSubdivisionTargetingExclusionRuleTest,
       AllowAdIfSubdivisionTargetingIsSupportedForMultipleGeoTargets) {
  // Arrange
  const URLEndpoints endpoints = {{R"(/v1/getstate)", {{net::HTTP_OK, R"(
            {"country":"US", "region":"FL"}
          )"}}}};
  MockUrlRequest(ads_client_mock_, endpoints);

  subdivision_targeting_->MaybeFetch();

  CreativeAdInfo creative_ad;
  creative_ad.creative_set_id = kCreativeSetId;
  creative_ad.geo_targets = {"US-FL", "US-CA"};

  // Act
  const bool should_exclude = exclusion_rule_->ShouldExclude(creative_ad);

  // Assert
  EXPECT_FALSE(should_exclude);
}

TEST_F(
    BatAdsSubdivisionTargetingExclusionRuleTest,
    AllowAdIfSubdivisionTargetingIsSupportedAndAutoDetectedForNonSubdivisionGeoTarget) {  // NOLINT
  // Arrange
  const URLEndpoints endpoints = {{R"(/v1/getstate)", {{net::HTTP_OK, R"(
            {"country":"US", "region":"FL"}
          )"}}}};
  MockUrlRequest(ads_client_mock_, endpoints);

  subdivision_targeting_->MaybeFetch();

  CreativeAdInfo creative_ad;
  creative_ad.creative_set_id = kCreativeSetId;
  creative_ad.geo_targets = {"US"};

  // Act
  const bool should_exclude = exclusion_rule_->ShouldExclude(creative_ad);

  // Assert
  EXPECT_FALSE(should_exclude);
}

TEST_F(BatAdsSubdivisionTargetingExclusionRuleTest,
       AllowAdIfSubdivisionTargetingIsSupportedAndManuallySelected) {
  // Arrange
  const URLEndpoints endpoints = {{R"(/v1/getstate)", {{net::HTTP_OK, R"(
            {"country":"US", "region":"FL"}
          )"}}}};
  MockUrlRequest(ads_client_mock_, endpoints);

  subdivision_targeting_->MaybeFetch();

  CreativeAdInfo creative_ad;
  creative_ad.creative_set_id = kCreativeSetId;
  creative_ad.geo_targets = {"US-FL"};

  // Act
  const bool should_exclude = exclusion_rule_->ShouldExclude(creative_ad);

  // Assert
  EXPECT_FALSE(should_exclude);
}

TEST_F(
    BatAdsSubdivisionTargetingExclusionRuleTest,
    AllowAdIfSubdivisionTargetingIsSupportedAndManuallySelectedForNonSubdivisionGeoTarget) {  // NOLINT
  // Arrange
  const URLEndpoints endpoints = {{R"(/v1/getstate)", {{net::HTTP_OK, R"(
            {"country":"US", "region":"FL"}
          )"}}}};
  MockUrlRequest(ads_client_mock_, endpoints);

  subdivision_targeting_->MaybeFetch();

  CreativeAdInfo creative_ad;
  creative_ad.creative_set_id = kCreativeSetId;
  creative_ad.geo_targets = {"US"};

  // Act
  const bool should_exclude = exclusion_rule_->ShouldExclude(creative_ad);

  // Assert
  EXPECT_FALSE(should_exclude);
}

TEST_F(BatAdsSubdivisionTargetingExclusionRuleTest,
       DoNotAllowAdIfSubdivisionTargetingIsNotSupportedOrNotInitialized) {
  // Arrange
  CreativeAdInfo creative_ad;
  creative_ad.creative_set_id = kCreativeSetId;
  creative_ad.geo_targets = {"US-FL"};

  // Act
  const bool should_exclude = exclusion_rule_->ShouldExclude(creative_ad);

  // Assert
  EXPECT_TRUE(should_exclude);
}

TEST_F(BatAdsSubdivisionTargetingExclusionRuleTest,
       DoNotAllowAdIfSubdivisionTargetingIsSupportedForUnsupportedGeoTarget) {
  // Arrange
  const URLEndpoints endpoints = {{R"(/v1/getstate)", {{net::HTTP_OK, R"(
            {"country":"US", "region":"FL"}
          )"}}}};
  MockUrlRequest(ads_client_mock_, endpoints);

  subdivision_targeting_->MaybeFetch();

  CreativeAdInfo creative_ad;
  creative_ad.creative_set_id = kCreativeSetId;
  creative_ad.geo_targets = {"US-XX"};

  // Act
  const bool should_exclude = exclusion_rule_->ShouldExclude(creative_ad);

  // Assert
  EXPECT_TRUE(should_exclude);
}

TEST_F(
    BatAdsSubdivisionTargetingExclusionRuleTest,
    DoNotAllowAdIfSubdivisionTargetingIsNotSupportedForSubdivisionGeoTarget) {
  // Arrange
  const URLEndpoints endpoints = {{R"(/v1/getstate)", {{net::HTTP_OK, R"(
            {"country":"US", "region":"FL"}
          )"}}}};
  MockUrlRequest(ads_client_mock_, endpoints);

  subdivision_targeting_->MaybeFetch();

  CreativeAdInfo creative_ad;
  creative_ad.creative_set_id = kCreativeSetId;
  creative_ad.geo_targets = {"GB-DEV"};

  // Act
  const bool should_exclude = exclusion_rule_->ShouldExclude(creative_ad);

  // Assert
  EXPECT_TRUE(should_exclude);
}

TEST_F(BatAdsSubdivisionTargetingExclusionRuleTest,
       AllowAdIfSubdivisionTargetingIsNotSupportedForNonSubdivisionGeoTarget) {
  // Arrange
  const URLEndpoints endpoints = {{R"(/v1/getstate)", {{net::HTTP_OK, R"(
            {"country":"XX", "region":"NO REGION"}
          )"}}}};
  MockUrlRequest(ads_client_mock_, endpoints);

  subdivision_targeting_->MaybeFetch();

  CreativeAdInfo creative_ad;
  creative_ad.creative_set_id = kCreativeSetId;
  creative_ad.geo_targets = {"XX"};

  // Act
  const bool should_exclude = exclusion_rule_->ShouldExclude(creative_ad);

  // Assert
  EXPECT_FALSE(should_exclude);
}

TEST_F(BatAdsSubdivisionTargetingExclusionRuleTest,
       DoNotAllowAdIfSubdivisionTargetingIsDisabledForSubdivisionGeoTarget) {
  // Arrange
  AdsClientHelper::GetInstance()->SetStringPref(
      prefs::kAdsSubdivisionTargetingCode, "DISABLED");

  const URLEndpoints endpoints = {{R"(/v1/getstate)", {{net::HTTP_OK, R"(
            {"country":"US", "region":"FL"}
          )"}}}};
  MockUrlRequest(ads_client_mock_, endpoints);

  subdivision_targeting_->MaybeFetch();

  CreativeAdInfo creative_ad;
  creative_ad.creative_set_id = kCreativeSetId;
  creative_ad.geo_targets = {"US-FL"};

  // Act
  const bool should_exclude = exclusion_rule_->ShouldExclude(creative_ad);

  // Assert
  EXPECT_TRUE(should_exclude);
}

TEST_F(BatAdsSubdivisionTargetingExclusionRuleTest,
       AllowAdIfSubdivisionTargetingIsDisabledForNonSubdivisionGeoTarget) {
  // Arrange
  AdsClientHelper::GetInstance()->SetStringPref(
      prefs::kAdsSubdivisionTargetingCode, "DISABLED");

  const URLEndpoints endpoints = {{R"(/v1/getstate)", {{net::HTTP_OK, R"(
            {"country":"US", "region":"FL"}
          )"}}}};
  MockUrlRequest(ads_client_mock_, endpoints);

  subdivision_targeting_->MaybeFetch();

  CreativeAdInfo creative_ad;
  creative_ad.creative_set_id = kCreativeSetId;
  creative_ad.geo_targets = {"US"};

  // Act
  const bool should_exclude = exclusion_rule_->ShouldExclude(creative_ad);

  // Assert
  EXPECT_FALSE(should_exclude);
}

}  // namespace ads
