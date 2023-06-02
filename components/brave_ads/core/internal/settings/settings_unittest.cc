/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/settings/settings.h"

#include <vector>

#include "base/test/scoped_feature_list.h"
#include "brave/components/brave_ads/common/notification_ad_feature.h"
#include "brave/components/brave_ads/common/pref_names.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_base.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_pref_util.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

class BraveAdsSettingsTest : public UnitTestBase {};

TEST_F(BraveAdsSettingsTest, AdsPerHourWhenUserHasChangedDefaultSetting) {
  // Arrange
  std::vector<base::test::FeatureRefAndParams> enabled_features;
  base::FieldTrialParams params;
  params["default_ads_per_hour"] = "2";
  enabled_features.emplace_back(kNotificationAdFeature, params);

  const std::vector<base::test::FeatureRef> disabled_features;

  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitWithFeaturesAndParameters(enabled_features,
                                                    disabled_features);

  SetDefaultInt64Pref(prefs::kMaximumNotificationAdsPerHour, 3);

  // Act

  // Assert
  EXPECT_EQ(3, GetMaximumNotificationAdsPerHourSetting());
}

TEST_F(BraveAdsSettingsTest, AdsPerHourWhenUserHasNotChangedDefaultSetting) {
  // Arrange
  std::vector<base::test::FeatureRefAndParams> enabled_features;
  base::FieldTrialParams params;
  params["default_ads_per_hour"] = "2";
  enabled_features.emplace_back(kNotificationAdFeature, params);

  const std::vector<base::test::FeatureRef> disabled_features;

  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitWithFeaturesAndParameters(enabled_features,
                                                    disabled_features);

  // Act

  // Assert
  EXPECT_EQ(2, GetMaximumNotificationAdsPerHourSetting());
}

}  // namespace brave_ads
