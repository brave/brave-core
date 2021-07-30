/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/frequency_capping/permission_rules/new_tab_page_ads_per_hour_frequency_cap.h"

#include <vector>

#include "base/feature_list.h"
#include "base/test/scoped_feature_list.h"
#include "bat/ads/internal/features/ad_serving/ad_serving_features.h"
#include "bat/ads/internal/frequency_capping/frequency_capping_unittest_util.h"
#include "bat/ads/internal/unittest_base.h"
#include "bat/ads/internal/unittest_util.h"

// npm run test -- brave_unit_tests --filter=BatAds*

namespace ads {

class BatAdsNewTabPageAdsPerHourFrequencyCapTest : public UnitTestBase {
 protected:
  BatAdsNewTabPageAdsPerHourFrequencyCapTest() = default;

  ~BatAdsNewTabPageAdsPerHourFrequencyCapTest() override = default;

  void SetUp() override {
    UnitTestBase::SetUp();

    const std::vector<base::test::ScopedFeatureList::FeatureAndParams>
        enabled_features;

    const std::vector<base::Feature> disabled_features;

    base::test::ScopedFeatureList scoped_feature_list;
    scoped_feature_list.InitWithFeaturesAndParameters(enabled_features,
                                                      disabled_features);
  }
};

TEST_F(BatAdsNewTabPageAdsPerHourFrequencyCapTest,
       AllowAdIfThereIsNoAdsHistory) {
  // Arrange

  // Act
  NewTabPageAdsPerHourFrequencyCap frequency_cap;
  const bool is_allowed = frequency_cap.ShouldAllow();

  // Assert
  EXPECT_TRUE(is_allowed);
}

TEST_F(BatAdsNewTabPageAdsPerHourFrequencyCapTest, AllowAdIfDoesNotExceedCap) {
  // Arrange
  const int count = features::GetMaximumNewTabPageAdsPerHour() - 1;
  RecordAdEvents(AdType::kNewTabPageAd, ConfirmationType::kServed, count);

  // Act
  NewTabPageAdsPerHourFrequencyCap frequency_cap;
  const bool is_allowed = frequency_cap.ShouldAllow();

  // Assert
  EXPECT_TRUE(is_allowed);
}

TEST_F(BatAdsNewTabPageAdsPerHourFrequencyCapTest,
       AllowAdIfDoesNotExceedCapAfter1Hour) {
  // Arrange
  const int count = features::GetMaximumNewTabPageAdsPerHour();
  RecordAdEvents(AdType::kNewTabPageAd, ConfirmationType::kServed, count);

  FastForwardClockBy(base::TimeDelta::FromHours(1));

  // Act
  NewTabPageAdsPerHourFrequencyCap frequency_cap;
  const bool is_allowed = frequency_cap.ShouldAllow();

  // Assert
  EXPECT_TRUE(is_allowed);
}

TEST_F(BatAdsNewTabPageAdsPerHourFrequencyCapTest,
       DoNotAllowAdIfExceedsCapWithin1Hour) {
  // Arrange
  const int count = features::GetMaximumNewTabPageAdsPerHour();
  RecordAdEvents(AdType::kNewTabPageAd, ConfirmationType::kServed, count);

  FastForwardClockBy(base::TimeDelta::FromMinutes(59));

  // Act
  NewTabPageAdsPerHourFrequencyCap frequency_cap;
  const bool is_allowed = frequency_cap.ShouldAllow();

  // Assert
  EXPECT_FALSE(is_allowed);
}

}  // namespace ads
