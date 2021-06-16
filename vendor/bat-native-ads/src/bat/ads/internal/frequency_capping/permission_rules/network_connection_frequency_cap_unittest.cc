/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/frequency_capping/permission_rules/network_connection_frequency_cap.h"

#include <vector>

#include "base/test/scoped_feature_list.h"
#include "bat/ads/internal/frequency_capping/frequency_capping_features.h"
#include "bat/ads/internal/unittest_base.h"
#include "bat/ads/internal/unittest_util.h"

// npm run test -- brave_unit_tests --filter=BatAds*

namespace ads {

class BatAdsNetworkConnectionFrequencyCapTest : public UnitTestBase {
 protected:
  BatAdsNetworkConnectionFrequencyCapTest() = default;

  ~BatAdsNetworkConnectionFrequencyCapTest() override = default;
};

TEST_F(BatAdsNetworkConnectionFrequencyCapTest, AllowAd) {
  // Arrange
  MockIsNetworkConnectionAvailable(ads_client_mock_, true);

  // Act
  NetworkConnectionFrequencyCap frequency_cap;
  const bool is_allowed = frequency_cap.ShouldAllow();

  // Assert
  EXPECT_TRUE(is_allowed);
}

TEST_F(BatAdsNetworkConnectionFrequencyCapTest, DoNotAllowAd) {
  // Arrange
  MockIsNetworkConnectionAvailable(ads_client_mock_, false);

  // Act
  NetworkConnectionFrequencyCap frequency_cap;
  const bool is_allowed = frequency_cap.ShouldAllow();

  // Assert
  EXPECT_FALSE(is_allowed);
}

TEST_F(BatAdsNetworkConnectionFrequencyCapTest,
       AllowAdIfFrequencyCapIsDisabled) {
  // Arrange
  base::FieldTrialParams kParameters;
  kParameters["should_only_serve_ads_with_valid_internet_connection"] = "false";
  std::vector<base::test::ScopedFeatureList::FeatureAndParams> enabled_features;
  enabled_features.push_back(
      {features::frequency_capping::kFeature, kParameters});

  const std::vector<base::Feature> disabled_features;

  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitWithFeaturesAndParameters(enabled_features,
                                                    disabled_features);

  MockIsNetworkConnectionAvailable(ads_client_mock_, false);

  // Act
  NetworkConnectionFrequencyCap frequency_cap;
  const bool is_allowed = frequency_cap.ShouldAllow();

  // Assert
  EXPECT_TRUE(is_allowed);
}

}  // namespace ads
