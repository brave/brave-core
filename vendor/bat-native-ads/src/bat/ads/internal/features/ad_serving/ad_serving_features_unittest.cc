/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/features/ad_serving/ad_serving_features.h"

#include <vector>

#include "base/feature_list.h"
#include "base/test/scoped_feature_list.h"
#include "testing/gtest/include/gtest/gtest.h"

// npm run test -- brave_unit_tests --filter=BatAds*

namespace ads {

TEST(BatAdsAdServingFeaturesTest, AdServingEnabled) {
  // Arrange

  // Act
  const bool is_enabled = features::IsAdServingEnabled();

  // Assert
  EXPECT_TRUE(is_enabled);
}

TEST(BatAdsAdServingFeaturesTest, AdServingDisabled) {
  // Arrange
  const std::vector<base::test::ScopedFeatureList::FeatureAndParams>
      enabled_features;

  std::vector<base::Feature> disabled_features;
  disabled_features.push_back(features::kAdServing);

  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitWithFeaturesAndParameters(enabled_features,
                                                    disabled_features);

  // Act
  const bool is_enabled = features::IsAdServingEnabled();

  // Assert
  EXPECT_FALSE(is_enabled);
}

TEST(BatAdsAdServingFeaturesTest, DefaultAdNotificationsPerHour) {
  // Arrange
  std::vector<base::test::ScopedFeatureList::FeatureAndParams> enabled_features;
  base::FieldTrialParams kAdServingParameters;
  kAdServingParameters["default_ad_notifications_per_hour"] = "2";
  enabled_features.push_back({features::kAdServing, kAdServingParameters});

  const std::vector<base::Feature> disabled_features;

  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitWithFeaturesAndParameters(enabled_features,
                                                    disabled_features);

  // Act
  const int default_ad_notifications_per_hour =
      features::GetDefaultAdNotificationsPerHour();

  // Assert
  const int expected_default_ad_notifications_per_hour = 2;
  EXPECT_EQ(expected_default_ad_notifications_per_hour,
            default_ad_notifications_per_hour);
}

TEST(BatAdsAdServingFeaturesTest, DefaultDefaultAdNotificationsPerHour) {
  // Arrange
  const std::vector<base::test::ScopedFeatureList::FeatureAndParams>
      enabled_features;

  const std::vector<base::Feature> disabled_features;

  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitWithFeaturesAndParameters(enabled_features,
                                                    disabled_features);
  // Act
  const int default_ad_notifications_per_hour =
      features::GetDefaultAdNotificationsPerHour();

  // Assert
  const int expected_default_ad_notifications_per_hour = 5;
  EXPECT_EQ(expected_default_ad_notifications_per_hour,
            default_ad_notifications_per_hour);
}

TEST(BatAdsAdServingFeaturesTest, DisabledDefaultAdNotificationsPerHour) {
  // Arrange
  const std::vector<base::test::ScopedFeatureList::FeatureAndParams>
      enabled_features;

  std::vector<base::Feature> disabled_features;
  disabled_features.push_back(features::kAdServing);

  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitWithFeaturesAndParameters(enabled_features,
                                                    disabled_features);

  // Act
  const int default_ad_notifications_per_hour =
      features::GetDefaultAdNotificationsPerHour();

  // Assert
  const int expected_default_ad_notifications_per_hour = 5;
  EXPECT_EQ(expected_default_ad_notifications_per_hour,
            default_ad_notifications_per_hour);
}

TEST(BatAdsAdServingFeaturesTest, MaximumAdNotificationsPerDay) {
  // Arrange
  std::vector<base::test::ScopedFeatureList::FeatureAndParams> enabled_features;
  base::FieldTrialParams kAdServingParameters;
  kAdServingParameters["maximum_ad_notifications_per_day"] = "7";
  enabled_features.push_back({features::kAdServing, kAdServingParameters});

  const std::vector<base::Feature> disabled_features;

  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitWithFeaturesAndParameters(enabled_features,
                                                    disabled_features);

  // Act
  const int maximum_ad_notifications_per_day =
      features::GetMaximumAdNotificationsPerDay();

  // Assert
  const int expected_maximum_ad_notifications_per_day = 7;
  EXPECT_EQ(expected_maximum_ad_notifications_per_day,
            maximum_ad_notifications_per_day);
}

TEST(BatAdsAdServingFeaturesTest, DefaultMaximumAdNotificationsPerDay) {
  // Arrange
  const std::vector<base::test::ScopedFeatureList::FeatureAndParams>
      enabled_features;

  const std::vector<base::Feature> disabled_features;

  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitWithFeaturesAndParameters(enabled_features,
                                                    disabled_features);
  // Act
  const int maximum_ad_notifications_per_day =
      features::GetMaximumAdNotificationsPerDay();

  // Assert
  const int expected_maximum_ad_notifications_per_day = 40;
  EXPECT_EQ(expected_maximum_ad_notifications_per_day,
            maximum_ad_notifications_per_day);
}

TEST(BatAdsAdServingFeaturesTest, DisabledMaximumAdNotificationsPerDay) {
  // Arrange
  const std::vector<base::test::ScopedFeatureList::FeatureAndParams>
      enabled_features;

  std::vector<base::Feature> disabled_features;
  disabled_features.push_back(features::kAdServing);

  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitWithFeaturesAndParameters(enabled_features,
                                                    disabled_features);

  // Act
  const int maximum_ad_notifications_per_day =
      features::GetMaximumAdNotificationsPerDay();

  // Assert
  const int expected_maximum_ad_notifications_per_day = 40;
  EXPECT_EQ(expected_maximum_ad_notifications_per_day,
            maximum_ad_notifications_per_day);
}

TEST(BatAdsAdServingFeaturesTest, MaximumInlineContentAdsPerHour) {
  // Arrange
  std::vector<base::test::ScopedFeatureList::FeatureAndParams> enabled_features;
  base::FieldTrialParams kAdServingParameters;
  kAdServingParameters["maximum_inline_content_ads_per_hour"] = "21";
  enabled_features.push_back({features::kAdServing, kAdServingParameters});

  const std::vector<base::Feature> disabled_features;

  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitWithFeaturesAndParameters(enabled_features,
                                                    disabled_features);

  // Act
  const int maximum_inline_content_ads_per_hour =
      features::GetMaximumInlineContentAdsPerHour();

  // Assert
  const int expected_maximum_inline_content_ads_per_hour = 21;
  EXPECT_EQ(expected_maximum_inline_content_ads_per_hour,
            maximum_inline_content_ads_per_hour);
}

TEST(BatAdsAdServingFeaturesTest, DefaultMaximumInlineContentAdsPerHour) {
  // Arrange
  const std::vector<base::test::ScopedFeatureList::FeatureAndParams>
      enabled_features;

  const std::vector<base::Feature> disabled_features;

  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitWithFeaturesAndParameters(enabled_features,
                                                    disabled_features);
  // Act
  const int maximum_inline_content_ads_per_hour =
      features::GetMaximumInlineContentAdsPerHour();

  // Assert
  const int expected_maximum_inline_content_ads_per_hour = 4;
  EXPECT_EQ(expected_maximum_inline_content_ads_per_hour,
            maximum_inline_content_ads_per_hour);
}

TEST(BatAdsAdServingFeaturesTest, DisabledMaximumInlineContentAdsPerHour) {
  // Arrange
  const std::vector<base::test::ScopedFeatureList::FeatureAndParams>
      enabled_features;

  std::vector<base::Feature> disabled_features;
  disabled_features.push_back(features::kAdServing);

  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitWithFeaturesAndParameters(enabled_features,
                                                    disabled_features);

  // Act
  const int maximum_inline_content_ads_per_hour =
      features::GetMaximumInlineContentAdsPerHour();

  // Assert
  const int expected_maximum_inline_content_ads_per_hour = 4;
  EXPECT_EQ(expected_maximum_inline_content_ads_per_hour,
            maximum_inline_content_ads_per_hour);
}

TEST(BatAdsAdServingFeaturesTest, MaximumInlineContentAdsPerDay) {
  // Arrange
  std::vector<base::test::ScopedFeatureList::FeatureAndParams> enabled_features;
  base::FieldTrialParams kAdServingParameters;
  kAdServingParameters["maximum_inline_content_ads_per_day"] = "24";
  enabled_features.push_back({features::kAdServing, kAdServingParameters});

  const std::vector<base::Feature> disabled_features;

  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitWithFeaturesAndParameters(enabled_features,
                                                    disabled_features);

  // Act
  const int maximum_inline_content_ads_per_day =
      features::GetMaximumInlineContentAdsPerDay();

  // Assert
  const int expected_maximum_inline_content_ads_per_day = 24;
  EXPECT_EQ(expected_maximum_inline_content_ads_per_day,
            maximum_inline_content_ads_per_day);
}

TEST(BatAdsAdServingFeaturesTest, DefaultMaximumInlineContentAdsPerDay) {
  // Arrange
  const std::vector<base::test::ScopedFeatureList::FeatureAndParams>
      enabled_features;

  const std::vector<base::Feature> disabled_features;

  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitWithFeaturesAndParameters(enabled_features,
                                                    disabled_features);
  // Act
  const int maximum_inline_content_ads_per_day =
      features::GetMaximumInlineContentAdsPerDay();

  // Assert
  const int expected_maximum_inline_content_ads_per_day = 20;
  EXPECT_EQ(expected_maximum_inline_content_ads_per_day,
            maximum_inline_content_ads_per_day);
}

TEST(BatAdsAdServingFeaturesTest, DisabledMaximumInlineContentAdsPerDay) {
  // Arrange
  const std::vector<base::test::ScopedFeatureList::FeatureAndParams>
      enabled_features;

  std::vector<base::Feature> disabled_features;
  disabled_features.push_back(features::kAdServing);

  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitWithFeaturesAndParameters(enabled_features,
                                                    disabled_features);

  // Act
  const int maximum_inline_content_ads_per_day =
      features::GetMaximumInlineContentAdsPerDay();

  // Assert
  const int expected_maximum_inline_content_ads_per_day = 20;
  EXPECT_EQ(expected_maximum_inline_content_ads_per_day,
            maximum_inline_content_ads_per_day);
}

TEST(BatAdsAdServingFeaturesTest, MaximumNewTabPageAdsPerHour) {
  // Arrange
  std::vector<base::test::ScopedFeatureList::FeatureAndParams> enabled_features;
  base::FieldTrialParams kAdServingParameters;
  kAdServingParameters["maximum_new_tab_page_ads_per_hour"] = "42";
  enabled_features.push_back({features::kAdServing, kAdServingParameters});

  const std::vector<base::Feature> disabled_features;

  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitWithFeaturesAndParameters(enabled_features,
                                                    disabled_features);

  // Act
  const int maximum_new_tab_page_ads_per_hour =
      features::GetMaximumNewTabPageAdsPerHour();

  // Assert
  const int expected_maximum_new_tab_page_ads_per_hour = 42;
  EXPECT_EQ(expected_maximum_new_tab_page_ads_per_hour,
            maximum_new_tab_page_ads_per_hour);
}

TEST(BatAdsAdServingFeaturesTest, DefaultMaximumNewTabPageAdsPerHour) {
  // Arrange
  const std::vector<base::test::ScopedFeatureList::FeatureAndParams>
      enabled_features;

  const std::vector<base::Feature> disabled_features;

  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitWithFeaturesAndParameters(enabled_features,
                                                    disabled_features);
  // Act
  const int maximum_new_tab_page_ads_per_hour =
      features::GetMaximumNewTabPageAdsPerHour();

  // Assert
  const int expected_maximum_new_tab_page_ads_per_hour = 4;
  EXPECT_EQ(expected_maximum_new_tab_page_ads_per_hour,
            maximum_new_tab_page_ads_per_hour);
}

TEST(BatAdsAdServingFeaturesTest, DisabledMaximumNewTabPageAdsPerHour) {
  // Arrange
  const std::vector<base::test::ScopedFeatureList::FeatureAndParams>
      enabled_features;

  std::vector<base::Feature> disabled_features;
  disabled_features.push_back(features::kAdServing);

  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitWithFeaturesAndParameters(enabled_features,
                                                    disabled_features);

  // Act
  const int maximum_new_tab_page_ads_per_hour =
      features::GetMaximumNewTabPageAdsPerHour();

  // Assert
  const int expected_maximum_new_tab_page_ads_per_hour = 4;
  EXPECT_EQ(expected_maximum_new_tab_page_ads_per_hour,
            maximum_new_tab_page_ads_per_hour);
}

TEST(BatAdsAdServingFeaturesTest, MaximumNewTabPageAdsPerDay) {
  // Arrange
  std::vector<base::test::ScopedFeatureList::FeatureAndParams> enabled_features;
  base::FieldTrialParams kAdServingParameters;
  kAdServingParameters["maximum_new_tab_page_ads_per_day"] = "24";
  enabled_features.push_back({features::kAdServing, kAdServingParameters});

  const std::vector<base::Feature> disabled_features;

  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitWithFeaturesAndParameters(enabled_features,
                                                    disabled_features);

  // Act
  const int maximum_new_tab_page_ads_per_day =
      features::GetMaximumNewTabPageAdsPerDay();

  // Assert
  const int expected_maximum_new_tab_page_ads_per_day = 24;
  EXPECT_EQ(expected_maximum_new_tab_page_ads_per_day,
            maximum_new_tab_page_ads_per_day);
}

TEST(BatAdsAdServingFeaturesTest, DefaultMaximumNewTabPageAdsPerDay) {
  // Arrange
  const std::vector<base::test::ScopedFeatureList::FeatureAndParams>
      enabled_features;

  const std::vector<base::Feature> disabled_features;

  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitWithFeaturesAndParameters(enabled_features,
                                                    disabled_features);
  // Act
  const int maximum_new_tab_page_ads_per_day =
      features::GetMaximumNewTabPageAdsPerDay();

  // Assert
  const int expected_maximum_new_tab_page_ads_per_day = 20;
  EXPECT_EQ(expected_maximum_new_tab_page_ads_per_day,
            maximum_new_tab_page_ads_per_day);
}

TEST(BatAdsAdServingFeaturesTest, DisabledMaximumNewTabPageAdsPerDay) {
  // Arrange
  const std::vector<base::test::ScopedFeatureList::FeatureAndParams>
      enabled_features;

  std::vector<base::Feature> disabled_features;
  disabled_features.push_back(features::kAdServing);

  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitWithFeaturesAndParameters(enabled_features,
                                                    disabled_features);

  // Act
  const int maximum_new_tab_page_ads_per_day =
      features::GetMaximumNewTabPageAdsPerDay();

  // Assert
  const int expected_maximum_new_tab_page_ads_per_day = 20;
  EXPECT_EQ(expected_maximum_new_tab_page_ads_per_day,
            maximum_new_tab_page_ads_per_day);
}

TEST(BatAdsAdServingFeaturesTest, MaximumPromotedContentAdsPerHour) {
  // Arrange
  std::vector<base::test::ScopedFeatureList::FeatureAndParams> enabled_features;
  base::FieldTrialParams kAdServingParameters;
  kAdServingParameters["maximum_promoted_content_ads_per_hour"] = "21";
  enabled_features.push_back({features::kAdServing, kAdServingParameters});

  const std::vector<base::Feature> disabled_features;

  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitWithFeaturesAndParameters(enabled_features,
                                                    disabled_features);

  // Act
  const int maximum_promoted_content_ads_per_hour =
      features::GetMaximumPromotedContentAdsPerHour();

  // Assert
  const int expected_maximum_promoted_content_ads_per_hour = 21;
  EXPECT_EQ(expected_maximum_promoted_content_ads_per_hour,
            maximum_promoted_content_ads_per_hour);
}

TEST(BatAdsAdServingFeaturesTest, DefaultMaximumPromotedContentAdsPerHour) {
  // Arrange
  const std::vector<base::test::ScopedFeatureList::FeatureAndParams>
      enabled_features;

  const std::vector<base::Feature> disabled_features;

  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitWithFeaturesAndParameters(enabled_features,
                                                    disabled_features);
  // Act
  const int maximum_promoted_content_ads_per_hour =
      features::GetMaximumPromotedContentAdsPerHour();

  // Assert
  const int expected_maximum_promoted_content_ads_per_hour = 4;
  EXPECT_EQ(expected_maximum_promoted_content_ads_per_hour,
            maximum_promoted_content_ads_per_hour);
}

TEST(BatAdsAdServingFeaturesTest, DisabledMaximumPromotedContentAdsPerHour) {
  // Arrange
  const std::vector<base::test::ScopedFeatureList::FeatureAndParams>
      enabled_features;

  std::vector<base::Feature> disabled_features;
  disabled_features.push_back(features::kAdServing);

  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitWithFeaturesAndParameters(enabled_features,
                                                    disabled_features);

  // Act
  const int maximum_promoted_content_ads_per_hour =
      features::GetMaximumPromotedContentAdsPerHour();

  // Assert
  const int expected_maximum_promoted_content_ads_per_hour = 4;
  EXPECT_EQ(expected_maximum_promoted_content_ads_per_hour,
            maximum_promoted_content_ads_per_hour);
}

TEST(BatAdsAdServingFeaturesTest, MaximumPromotedContentAdsPerDay) {
  // Arrange
  std::vector<base::test::ScopedFeatureList::FeatureAndParams> enabled_features;
  base::FieldTrialParams kAdServingParameters;
  kAdServingParameters["maximum_promoted_content_ads_per_day"] = "24";
  enabled_features.push_back({features::kAdServing, kAdServingParameters});

  const std::vector<base::Feature> disabled_features;

  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitWithFeaturesAndParameters(enabled_features,
                                                    disabled_features);

  // Act
  const int maximum_promoted_content_ads_per_day =
      features::GetMaximumPromotedContentAdsPerDay();

  // Assert
  const int expected_maximum_promoted_content_ads_per_day = 24;
  EXPECT_EQ(expected_maximum_promoted_content_ads_per_day,
            maximum_promoted_content_ads_per_day);
}

TEST(BatAdsAdServingFeaturesTest, DefaultMaximumPromotedContentAdsPerDay) {
  // Arrange
  const std::vector<base::test::ScopedFeatureList::FeatureAndParams>
      enabled_features;

  const std::vector<base::Feature> disabled_features;

  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitWithFeaturesAndParameters(enabled_features,
                                                    disabled_features);
  // Act
  const int maximum_promoted_content_ads_per_day =
      features::GetMaximumPromotedContentAdsPerDay();

  // Assert
  const int expected_maximum_promoted_content_ads_per_day = 20;
  EXPECT_EQ(expected_maximum_promoted_content_ads_per_day,
            maximum_promoted_content_ads_per_day);
}

TEST(BatAdsAdServingFeaturesTest, DisabledMaximumPromotedContentAdsPerDay) {
  // Arrange
  const std::vector<base::test::ScopedFeatureList::FeatureAndParams>
      enabled_features;

  std::vector<base::Feature> disabled_features;
  disabled_features.push_back(features::kAdServing);

  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitWithFeaturesAndParameters(enabled_features,
                                                    disabled_features);

  // Act
  const int maximum_promoted_content_ads_per_day =
      features::GetMaximumPromotedContentAdsPerDay();

  // Assert
  const int expected_maximum_promoted_content_ads_per_day = 20;
  EXPECT_EQ(expected_maximum_promoted_content_ads_per_day,
            maximum_promoted_content_ads_per_day);
}

}  // namespace ads
