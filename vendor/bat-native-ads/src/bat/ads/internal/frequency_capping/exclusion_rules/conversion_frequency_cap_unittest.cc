/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/frequency_capping/exclusion_rules/conversion_frequency_cap.h"

#include <vector>

#include "bat/ads/internal/frequency_capping/frequency_capping_unittest_util.h"
#include "bat/ads/internal/unittest_base.h"
#include "bat/ads/internal/unittest_util.h"
#include "bat/ads/pref_names.h"

// npm run test -- brave_unit_tests --filter=BatAds*

namespace ads {

namespace {

const std::vector<std::string> kCreativeSetIds = {
  "654f10df-fbc4-4a92-8d43-2edf73734a60",
  "465f10df-fbc4-4a92-8d43-4edf73734a60"
};

}  // namespace

class BatAdsConversionFrequencyCapTest : public UnitTestBase {
 protected:
  BatAdsConversionFrequencyCapTest() = default;

  ~BatAdsConversionFrequencyCapTest() override = default;
};

TEST_F(BatAdsConversionFrequencyCapTest,
    AllowAdIfThereIsNoConversionHistory) {
  // Arrange
  CreativeAdInfo ad;
  ad.creative_set_id = "654f10df-fbc4-4a92-8d43-2edf73734a60";

  const AdEventList ad_events;

  // Act
  ConversionFrequencyCap frequency_cap(ad_events);
  const bool should_exclude = frequency_cap.ShouldExclude(ad);

  // Assert
  EXPECT_FALSE(should_exclude);
}

TEST_F(BatAdsConversionFrequencyCapTest,
    DoNotAllowAdIfShouldNotAllowConversionTracking) {
  // Arrange
  ads_client_mock_->SetBooleanPref(
      prefs::kShouldAllowConversionTracking, false);

  CreativeAdInfo ad;
  ad.creative_set_id = kCreativeSetIds.at(0);

  AdEventList ad_events;

  const AdEventInfo ad_event = GenerateAdEvent(AdType::kAdNotification, ad,
      ConfirmationType::kConversion);

  ad_events.push_back(ad_event);

  // Act
  ConversionFrequencyCap frequency_cap(ad_events);
  const bool should_exclude = frequency_cap.ShouldExclude(ad);

  // Assert
  EXPECT_TRUE(should_exclude);
}

TEST_F(BatAdsConversionFrequencyCapTest,
    DoNotAllowAdIfAlreadyConverted) {
  // Arrange
  CreativeAdInfo ad;
  ad.creative_set_id = kCreativeSetIds.at(0);

  AdEventList ad_events;

  const AdEventInfo ad_event = GenerateAdEvent(AdType::kAdNotification, ad,
      ConfirmationType::kConversion);

  ad_events.push_back(ad_event);

  // Act
  ConversionFrequencyCap frequency_cap(ad_events);
  const bool should_exclude = frequency_cap.ShouldExclude(ad);

  // Assert
  EXPECT_TRUE(should_exclude);
}

TEST_F(BatAdsConversionFrequencyCapTest,
    AllowAdIfNotAlreadyConverted) {
  // Arrange
  CreativeAdInfo ad_1;
  ad_1.creative_set_id = kCreativeSetIds.at(0);

  CreativeAdInfo ad_2;
  ad_2.creative_set_id = kCreativeSetIds.at(1);

  AdEventList ad_events;

  const AdEventInfo ad_event = GenerateAdEvent(AdType::kAdNotification, ad_2,
      ConfirmationType::kConversion);

  ad_events.push_back(ad_event);

  // Act
  ConversionFrequencyCap frequency_cap(ad_events);
  const bool should_exclude = frequency_cap.ShouldExclude(ad_1);

  // Assert
  EXPECT_FALSE(should_exclude);
}

}  // namespace ads
