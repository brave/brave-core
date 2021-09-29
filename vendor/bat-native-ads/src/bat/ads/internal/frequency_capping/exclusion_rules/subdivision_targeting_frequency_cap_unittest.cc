/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/frequency_capping/exclusion_rules/subdivision_targeting_frequency_cap.h"

#include <memory>

#include "bat/ads/internal/ad_serving/ad_targeting/geographic/subdivision/subdivision_targeting.h"
#include "bat/ads/internal/unittest_base.h"
#include "bat/ads/internal/unittest_util.h"
#include "bat/ads/pref_names.h"

// npm run test -- brave_unit_tests --filter=BatAds*

namespace ads {

namespace {
const char kCreativeSetId[] = "654f10df-fbc4-4a92-8d43-2edf73734a60";
}  // namespace

class BatAdsSubdivisionTargetingFrequencyCapTest : public UnitTestBase {
 protected:
  BatAdsSubdivisionTargetingFrequencyCapTest()
      : subdivision_targeting_(
            std::make_unique<ad_targeting::geographic::SubdivisionTargeting>()),
        frequency_cap_(std::make_unique<SubdivisionTargetingFrequencyCap>(
            subdivision_targeting_.get())) {}

  ~BatAdsSubdivisionTargetingFrequencyCapTest() override = default;

  std::unique_ptr<ad_targeting::geographic::SubdivisionTargeting>
      subdivision_targeting_;
  std::unique_ptr<SubdivisionTargetingFrequencyCap> frequency_cap_;
};

TEST_F(BatAdsSubdivisionTargetingFrequencyCapTest,
       AllowAdIfSubdivisionTargetingIsSupportedAndAutoDetected) {
  // Arrange
  ads_client_mock_->SetStringPref(
      prefs::kAutoDetectedAdsSubdivisionTargetingCode, "US-FL");

  ads_client_mock_->SetStringPref(prefs::kAdsSubdivisionTargetingCode, "AUTO");

  CreativeAdInfo ad;
  ad.creative_set_id = kCreativeSetId;
  ad.geo_targets = {"US-FL"};

  // Act
  const bool should_exclude = frequency_cap_->ShouldExclude(ad);

  // Assert
  EXPECT_FALSE(should_exclude);
}

TEST_F(
    BatAdsSubdivisionTargetingFrequencyCapTest,
    AllowAdIfSubdivisionTargetingIsSupportedAndAutoDetectedForNonSubdivisionGeoTarget) {  // NOLINT
  // Arrange
  ads_client_mock_->SetStringPref(
      prefs::kAutoDetectedAdsSubdivisionTargetingCode, "US-FL");

  ads_client_mock_->SetStringPref(prefs::kAdsSubdivisionTargetingCode, "AUTO");

  CreativeAdInfo ad;
  ad.creative_set_id = kCreativeSetId;
  ad.geo_targets = {"US"};

  // Act
  const bool should_exclude = frequency_cap_->ShouldExclude(ad);

  // Assert
  EXPECT_FALSE(should_exclude);
}

TEST_F(BatAdsSubdivisionTargetingFrequencyCapTest,
       AllowAdIfSubdivisionTargetingIsSupportedAndManuallySelected) {
  // Arrange
  ads_client_mock_->SetStringPref(prefs::kAdsSubdivisionTargetingCode, "US-FL");

  CreativeAdInfo ad;
  ad.creative_set_id = kCreativeSetId;
  ad.geo_targets = {"US-FL"};

  // Act
  const bool should_exclude = frequency_cap_->ShouldExclude(ad);

  // Assert
  EXPECT_FALSE(should_exclude);
}

TEST_F(
    BatAdsSubdivisionTargetingFrequencyCapTest,
    AllowAdIfSubdivisionTargetingIsSupportedAndManuallySelectedForNonSubdivisionGeoTarget) {  // NOLINT
  // Arrange
  ads_client_mock_->SetStringPref(prefs::kAdsSubdivisionTargetingCode, "US-FL");

  CreativeAdInfo ad;
  ad.creative_set_id = kCreativeSetId;
  ad.geo_targets = {"US"};

  // Act
  const bool should_exclude = frequency_cap_->ShouldExclude(ad);

  // Assert
  EXPECT_FALSE(should_exclude);
}

TEST_F(BatAdsSubdivisionTargetingFrequencyCapTest,
       DoNotAllowAdIfSubdivisionTargetingIsSupportedAndNotInitialized) {
  // Arrange
  ads_client_mock_->SetStringPref(
      prefs::kAutoDetectedAdsSubdivisionTargetingCode, "");

  ads_client_mock_->SetStringPref(prefs::kAdsSubdivisionTargetingCode, "AUTO");

  CreativeAdInfo ad;
  ad.creative_set_id = kCreativeSetId;
  ad.geo_targets = {"US-FL"};

  // Act
  const bool should_exclude = frequency_cap_->ShouldExclude(ad);

  // Assert
  EXPECT_TRUE(should_exclude);
}

TEST_F(BatAdsSubdivisionTargetingFrequencyCapTest,
       DoNotAllowAdIfSubdivisionTargetingIsSupportedForUnsupportedGeoTarget) {
  // Arrange
  ads_client_mock_->SetStringPref(prefs::kAdsSubdivisionTargetingCode, "US-FL");

  CreativeAdInfo ad;
  ad.creative_set_id = kCreativeSetId;
  ad.geo_targets = {"US-XX"};

  // Act
  const bool should_exclude = frequency_cap_->ShouldExclude(ad);

  // Assert
  EXPECT_TRUE(should_exclude);
}

TEST_F(
    BatAdsSubdivisionTargetingFrequencyCapTest,
    DoNotAllowAdIfSubdivisionTargetingIsNotSupportedForSubdivisionGeoTarget) {
  // Arrange
  MockLocaleHelper(locale_helper_mock_, "en-XX");

  CreativeAdInfo ad;
  ad.creative_set_id = kCreativeSetId;
  ad.geo_targets = {"XX-DEV"};

  // Act
  const bool should_exclude = frequency_cap_->ShouldExclude(ad);

  // Assert
  EXPECT_TRUE(should_exclude);
}

TEST_F(BatAdsSubdivisionTargetingFrequencyCapTest,
       AllowAdIfSubdivisionTargetingIsNotSupportedForNonSubdivisionGeoTarget) {
  // Arrange
  MockLocaleHelper(locale_helper_mock_, "en-XX");

  CreativeAdInfo ad;
  ad.creative_set_id = kCreativeSetId;
  ad.geo_targets = {"XX"};

  // Act
  const bool should_exclude = frequency_cap_->ShouldExclude(ad);

  // Assert
  EXPECT_FALSE(should_exclude);
}

TEST_F(BatAdsSubdivisionTargetingFrequencyCapTest,
       DoNotAllowAdIfSubdivisionTargetingIsDisabledForSubdivisionGeoTarget) {
  // Arrange
  ads_client_mock_->SetStringPref(prefs::kAdsSubdivisionTargetingCode,
                                  "DISABLED");

  CreativeAdInfo ad;
  ad.creative_set_id = kCreativeSetId;
  ad.geo_targets = {"US-FL"};

  // Act
  const bool should_exclude = frequency_cap_->ShouldExclude(ad);

  // Assert
  EXPECT_TRUE(should_exclude);
}

TEST_F(BatAdsSubdivisionTargetingFrequencyCapTest,
       AllowAdIfSubdivisionTargetingIsDisabledForNonSubdivisionGeoTarget) {
  // Arrange
  ads_client_mock_->SetStringPref(prefs::kAdsSubdivisionTargetingCode,
                                  "DISABLED");

  CreativeAdInfo ad;
  ad.creative_set_id = kCreativeSetId;
  ad.geo_targets = {"US"};

  // Act
  const bool should_exclude = frequency_cap_->ShouldExclude(ad);

  // Assert
  EXPECT_FALSE(should_exclude);
}

}  // namespace ads
