/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/eligible_ads/exclusion_rules/subdivision_targeting_exclusion_rule.h"

#include <memory>

#include "bat/ads/internal/base/unittest_base.h"
#include "bat/ads/internal/base/unittest_util.h"
#include "bat/ads/internal/serving/targeting/geographic/subdivision/subdivision_targeting.h"
#include "bat/ads/pref_names.h"

// npm run test -- brave_unit_tests --filter=BatAds*

namespace ads {

namespace {
constexpr char kCreativeSetId[] = "654f10df-fbc4-4a92-8d43-2edf73734a60";
}  // namespace

class BatAdsSubdivisionTargetingExclusionRuleTest : public UnitTestBase {
 protected:
  BatAdsSubdivisionTargetingExclusionRuleTest()
      : subdivision_targeting_(
            std::make_unique<targeting::geographic::SubdivisionTargeting>()),
        frequency_cap_(std::make_unique<SubdivisionTargetingExclusionRule>(
            subdivision_targeting_.get())) {}

  ~BatAdsSubdivisionTargetingExclusionRuleTest() override = default;

  std::unique_ptr<targeting::geographic::SubdivisionTargeting>
      subdivision_targeting_;
  std::unique_ptr<SubdivisionTargetingExclusionRule> frequency_cap_;
};

TEST_F(BatAdsSubdivisionTargetingExclusionRuleTest,
       AllowAdIfSubdivisionTargetingIsSupportedAndAutoDetected) {
  // Arrange
  ads_client_mock_->SetStringPref(
      prefs::kAutoDetectedAdsSubdivisionTargetingCode, "US-FL");

  ads_client_mock_->SetStringPref(prefs::kAdsSubdivisionTargetingCode, "AUTO");

  CreativeAdInfo creative_ad;
  creative_ad.creative_set_id = kCreativeSetId;
  creative_ad.geo_targets = {"US-FL"};

  // Act
  const bool should_exclude = frequency_cap_->ShouldExclude(creative_ad);

  // Assert
  EXPECT_FALSE(should_exclude);
}

TEST_F(BatAdsSubdivisionTargetingExclusionRuleTest,
       AllowAdIfSubdivisionTargetingIsSupportedForMultipleGeoTargets) {
  // Arrange
  ads_client_mock_->SetStringPref(prefs::kAdsSubdivisionTargetingCode, "US-FL");

  CreativeAdInfo creative_ad;
  creative_ad.creative_set_id = kCreativeSetId;
  creative_ad.geo_targets = {"US-FL", "US-CA"};

  // Act
  const bool should_exclude = frequency_cap_->ShouldExclude(creative_ad);

  // Assert
  EXPECT_FALSE(should_exclude);
}

TEST_F(
    BatAdsSubdivisionTargetingExclusionRuleTest,
    AllowAdIfSubdivisionTargetingIsSupportedAndAutoDetectedForNonSubdivisionGeoTarget) {  // NOLINT
  // Arrange
  ads_client_mock_->SetStringPref(
      prefs::kAutoDetectedAdsSubdivisionTargetingCode, "US-FL");

  ads_client_mock_->SetStringPref(prefs::kAdsSubdivisionTargetingCode, "AUTO");

  CreativeAdInfo creative_ad;
  creative_ad.creative_set_id = kCreativeSetId;
  creative_ad.geo_targets = {"US"};

  // Act
  const bool should_exclude = frequency_cap_->ShouldExclude(creative_ad);

  // Assert
  EXPECT_FALSE(should_exclude);
}

TEST_F(BatAdsSubdivisionTargetingExclusionRuleTest,
       AllowAdIfSubdivisionTargetingIsSupportedAndManuallySelected) {
  // Arrange
  ads_client_mock_->SetStringPref(prefs::kAdsSubdivisionTargetingCode, "US-FL");

  CreativeAdInfo creative_ad;
  creative_ad.creative_set_id = kCreativeSetId;
  creative_ad.geo_targets = {"US-FL"};

  // Act
  const bool should_exclude = frequency_cap_->ShouldExclude(creative_ad);

  // Assert
  EXPECT_FALSE(should_exclude);
}

TEST_F(
    BatAdsSubdivisionTargetingExclusionRuleTest,
    AllowAdIfSubdivisionTargetingIsSupportedAndManuallySelectedForNonSubdivisionGeoTarget) {  // NOLINT
  // Arrange
  ads_client_mock_->SetStringPref(prefs::kAdsSubdivisionTargetingCode, "US-FL");

  CreativeAdInfo creative_ad;
  creative_ad.creative_set_id = kCreativeSetId;
  creative_ad.geo_targets = {"US"};

  // Act
  const bool should_exclude = frequency_cap_->ShouldExclude(creative_ad);

  // Assert
  EXPECT_FALSE(should_exclude);
}

TEST_F(BatAdsSubdivisionTargetingExclusionRuleTest,
       DoNotAllowAdIfSubdivisionTargetingIsSupportedAndNotInitialized) {
  // Arrange
  ads_client_mock_->SetStringPref(
      prefs::kAutoDetectedAdsSubdivisionTargetingCode, "");

  ads_client_mock_->SetStringPref(prefs::kAdsSubdivisionTargetingCode, "AUTO");

  CreativeAdInfo creative_ad;
  creative_ad.creative_set_id = kCreativeSetId;
  creative_ad.geo_targets = {"US-FL"};

  // Act
  const bool should_exclude = frequency_cap_->ShouldExclude(creative_ad);

  // Assert
  EXPECT_TRUE(should_exclude);
}

TEST_F(BatAdsSubdivisionTargetingExclusionRuleTest,
       DoNotAllowAdIfSubdivisionTargetingIsSupportedForUnsupportedGeoTarget) {
  // Arrange
  ads_client_mock_->SetStringPref(prefs::kAdsSubdivisionTargetingCode, "US-FL");

  CreativeAdInfo creative_ad;
  creative_ad.creative_set_id = kCreativeSetId;
  creative_ad.geo_targets = {"US-XX"};

  // Act
  const bool should_exclude = frequency_cap_->ShouldExclude(creative_ad);

  // Assert
  EXPECT_TRUE(should_exclude);
}

TEST_F(
    BatAdsSubdivisionTargetingExclusionRuleTest,
    DoNotAllowAdIfSubdivisionTargetingIsNotSupportedForSubdivisionGeoTarget) {
  // Arrange
  MockLocaleHelper(locale_helper_mock_, "en-XX");

  CreativeAdInfo creative_ad;
  creative_ad.creative_set_id = kCreativeSetId;
  creative_ad.geo_targets = {"XX-DEV"};

  // Act
  const bool should_exclude = frequency_cap_->ShouldExclude(creative_ad);

  // Assert
  EXPECT_TRUE(should_exclude);
}

TEST_F(BatAdsSubdivisionTargetingExclusionRuleTest,
       AllowAdIfSubdivisionTargetingIsNotSupportedForNonSubdivisionGeoTarget) {
  // Arrange
  MockLocaleHelper(locale_helper_mock_, "en-XX");

  CreativeAdInfo creative_ad;
  creative_ad.creative_set_id = kCreativeSetId;
  creative_ad.geo_targets = {"XX"};

  // Act
  const bool should_exclude = frequency_cap_->ShouldExclude(creative_ad);

  // Assert
  EXPECT_FALSE(should_exclude);
}

TEST_F(BatAdsSubdivisionTargetingExclusionRuleTest,
       DoNotAllowAdIfSubdivisionTargetingIsDisabledForSubdivisionGeoTarget) {
  // Arrange
  ads_client_mock_->SetStringPref(prefs::kAdsSubdivisionTargetingCode,
                                  "DISABLED");

  CreativeAdInfo creative_ad;
  creative_ad.creative_set_id = kCreativeSetId;
  creative_ad.geo_targets = {"US-FL"};

  // Act
  const bool should_exclude = frequency_cap_->ShouldExclude(creative_ad);

  // Assert
  EXPECT_TRUE(should_exclude);
}

TEST_F(BatAdsSubdivisionTargetingExclusionRuleTest,
       AllowAdIfSubdivisionTargetingIsDisabledForNonSubdivisionGeoTarget) {
  // Arrange
  ads_client_mock_->SetStringPref(prefs::kAdsSubdivisionTargetingCode,
                                  "DISABLED");

  CreativeAdInfo creative_ad;
  creative_ad.creative_set_id = kCreativeSetId;
  creative_ad.geo_targets = {"US"};

  // Act
  const bool should_exclude = frequency_cap_->ShouldExclude(creative_ad);

  // Assert
  EXPECT_FALSE(should_exclude);
}

}  // namespace ads
