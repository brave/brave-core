/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/serving/serving_features.h"

#include <vector>

#include "base/test/scoped_feature_list.h"
#include "testing/gtest/include/gtest/gtest.h"

// npm run test -- brave_unit_tests --filter=BatAds*

namespace ads {
namespace features {

TEST(BatAdsServingFeaturesTest, ServingEnabled) {
  // Arrange

  // Act
  const bool is_enabled = IsServingEnabled();

  // Assert
  EXPECT_TRUE(is_enabled);
}

TEST(BatAdsServingFeaturesTest, ServingDisabled) {
  // Arrange
  const std::vector<base::test::ScopedFeatureList::FeatureAndParams>
      enabled_features;

  std::vector<base::Feature> disabled_features;
  disabled_features.push_back(kServing);

  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitWithFeaturesAndParameters(enabled_features,
                                                    disabled_features);

  // Act
  const bool is_enabled = IsServingEnabled();

  // Assert
  EXPECT_FALSE(is_enabled);
}

TEST(BatAdsServingFeaturesTest, DefaultNotificationAdsPerHour) {
  // Arrange
  std::vector<base::test::ScopedFeatureList::FeatureAndParams> enabled_features;
  base::FieldTrialParams kServingParameters;
  kServingParameters["default_ad_notifications_per_hour"] = "2";
  enabled_features.push_back({kServing, kServingParameters});

  const std::vector<base::Feature> disabled_features;

  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitWithFeaturesAndParameters(enabled_features,
                                                    disabled_features);

  // Act
  const int default_notification_ads_per_hour =
      GetDefaultNotificationAdsPerHour();

  // Assert
  const int expected_default_notification_ads_per_hour = 2;
  EXPECT_EQ(expected_default_notification_ads_per_hour,
            default_notification_ads_per_hour);
}

TEST(BatAdsServingFeaturesTest, DefaultDefaultNotificationAdsPerHour) {
  // Arrange
  const std::vector<base::test::ScopedFeatureList::FeatureAndParams>
      enabled_features;

  const std::vector<base::Feature> disabled_features;

  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitWithFeaturesAndParameters(enabled_features,
                                                    disabled_features);
  // Act
  const int default_notification_ads_per_hour =
      GetDefaultNotificationAdsPerHour();

  // Assert
  const int expected_default_notification_ads_per_hour = 5;
  EXPECT_EQ(expected_default_notification_ads_per_hour,
            default_notification_ads_per_hour);
}

TEST(BatAdsServingFeaturesTest, DisabledDefaultNotificationAdsPerHour) {
  // Arrange
  const std::vector<base::test::ScopedFeatureList::FeatureAndParams>
      enabled_features;

  std::vector<base::Feature> disabled_features;
  disabled_features.push_back(kServing);

  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitWithFeaturesAndParameters(enabled_features,
                                                    disabled_features);

  // Act
  const int default_notification_ads_per_hour =
      GetDefaultNotificationAdsPerHour();

  // Assert
  const int expected_default_notification_ads_per_hour = 5;
  EXPECT_EQ(expected_default_notification_ads_per_hour,
            default_notification_ads_per_hour);
}

TEST(BatAdsServingFeaturesTest, MaximumNotificationAdsPerDay) {
  // Arrange
  std::vector<base::test::ScopedFeatureList::FeatureAndParams> enabled_features;
  base::FieldTrialParams kServingParameters;
  kServingParameters["maximum_ad_notifications_per_day"] = "7";
  enabled_features.push_back({kServing, kServingParameters});

  const std::vector<base::Feature> disabled_features;

  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitWithFeaturesAndParameters(enabled_features,
                                                    disabled_features);

  // Act
  const int maximum_notification_ads_per_day =
      GetMaximumNotificationAdsPerDay();

  // Assert
  const int expected_maximum_notification_ads_per_day = 7;
  EXPECT_EQ(expected_maximum_notification_ads_per_day,
            maximum_notification_ads_per_day);
}

TEST(BatAdsServingFeaturesTest, DefaultMaximumNotificationAdsPerDay) {
  // Arrange
  const std::vector<base::test::ScopedFeatureList::FeatureAndParams>
      enabled_features;

  const std::vector<base::Feature> disabled_features;

  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitWithFeaturesAndParameters(enabled_features,
                                                    disabled_features);
  // Act
  const int maximum_notification_ads_per_day =
      GetMaximumNotificationAdsPerDay();

  // Assert
  const int expected_maximum_notification_ads_per_day = 40;
  EXPECT_EQ(expected_maximum_notification_ads_per_day,
            maximum_notification_ads_per_day);
}

TEST(BatAdsServingFeaturesTest, DisabledMaximumNotificationAdsPerDay) {
  // Arrange
  const std::vector<base::test::ScopedFeatureList::FeatureAndParams>
      enabled_features;

  std::vector<base::Feature> disabled_features;
  disabled_features.push_back(kServing);

  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitWithFeaturesAndParameters(enabled_features,
                                                    disabled_features);

  // Act
  const int maximum_notification_ads_per_day =
      GetMaximumNotificationAdsPerDay();

  // Assert
  const int expected_maximum_notification_ads_per_day = 40;
  EXPECT_EQ(expected_maximum_notification_ads_per_day,
            maximum_notification_ads_per_day);
}

TEST(BatAdsServingFeaturesTest, MaximumInlineContentAdsPerHour) {
  // Arrange
  std::vector<base::test::ScopedFeatureList::FeatureAndParams> enabled_features;
  base::FieldTrialParams kServingParameters;
  kServingParameters["maximum_inline_content_ads_per_hour"] = "21";
  enabled_features.push_back({kServing, kServingParameters});

  const std::vector<base::Feature> disabled_features;

  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitWithFeaturesAndParameters(enabled_features,
                                                    disabled_features);

  // Act
  const int maximum_inline_content_ads_per_hour =
      GetMaximumInlineContentAdsPerHour();

  // Assert
  const int expected_maximum_inline_content_ads_per_hour = 21;
  EXPECT_EQ(expected_maximum_inline_content_ads_per_hour,
            maximum_inline_content_ads_per_hour);
}

TEST(BatAdsServingFeaturesTest, DefaultMaximumInlineContentAdsPerHour) {
  // Arrange
  const std::vector<base::test::ScopedFeatureList::FeatureAndParams>
      enabled_features;

  const std::vector<base::Feature> disabled_features;

  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitWithFeaturesAndParameters(enabled_features,
                                                    disabled_features);
  // Act
  const int maximum_inline_content_ads_per_hour =
      GetMaximumInlineContentAdsPerHour();

  // Assert
  const int expected_maximum_inline_content_ads_per_hour = 4;
  EXPECT_EQ(expected_maximum_inline_content_ads_per_hour,
            maximum_inline_content_ads_per_hour);
}

TEST(BatAdsServingFeaturesTest, DisabledMaximumInlineContentAdsPerHour) {
  // Arrange
  const std::vector<base::test::ScopedFeatureList::FeatureAndParams>
      enabled_features;

  std::vector<base::Feature> disabled_features;
  disabled_features.push_back(kServing);

  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitWithFeaturesAndParameters(enabled_features,
                                                    disabled_features);

  // Act
  const int maximum_inline_content_ads_per_hour =
      GetMaximumInlineContentAdsPerHour();

  // Assert
  const int expected_maximum_inline_content_ads_per_hour = 4;
  EXPECT_EQ(expected_maximum_inline_content_ads_per_hour,
            maximum_inline_content_ads_per_hour);
}

TEST(BatAdsServingFeaturesTest, MaximumInlineContentAdsPerDay) {
  // Arrange
  std::vector<base::test::ScopedFeatureList::FeatureAndParams> enabled_features;
  base::FieldTrialParams kServingParameters;
  kServingParameters["maximum_inline_content_ads_per_day"] = "24";
  enabled_features.push_back({kServing, kServingParameters});

  const std::vector<base::Feature> disabled_features;

  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitWithFeaturesAndParameters(enabled_features,
                                                    disabled_features);

  // Act
  const int maximum_inline_content_ads_per_day =
      GetMaximumInlineContentAdsPerDay();

  // Assert
  const int expected_maximum_inline_content_ads_per_day = 24;
  EXPECT_EQ(expected_maximum_inline_content_ads_per_day,
            maximum_inline_content_ads_per_day);
}

TEST(BatAdsServingFeaturesTest, DefaultMaximumInlineContentAdsPerDay) {
  // Arrange
  const std::vector<base::test::ScopedFeatureList::FeatureAndParams>
      enabled_features;

  const std::vector<base::Feature> disabled_features;

  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitWithFeaturesAndParameters(enabled_features,
                                                    disabled_features);
  // Act
  const int maximum_inline_content_ads_per_day =
      GetMaximumInlineContentAdsPerDay();

  // Assert
  const int expected_maximum_inline_content_ads_per_day = 20;
  EXPECT_EQ(expected_maximum_inline_content_ads_per_day,
            maximum_inline_content_ads_per_day);
}

TEST(BatAdsServingFeaturesTest, DisabledMaximumInlineContentAdsPerDay) {
  // Arrange
  const std::vector<base::test::ScopedFeatureList::FeatureAndParams>
      enabled_features;

  std::vector<base::Feature> disabled_features;
  disabled_features.push_back(kServing);

  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitWithFeaturesAndParameters(enabled_features,
                                                    disabled_features);

  // Act
  const int maximum_inline_content_ads_per_day =
      GetMaximumInlineContentAdsPerDay();

  // Assert
  const int expected_maximum_inline_content_ads_per_day = 20;
  EXPECT_EQ(expected_maximum_inline_content_ads_per_day,
            maximum_inline_content_ads_per_day);
}

TEST(BatAdsServingFeaturesTest, MaximumNewTabPageAdsPerHour) {
  // Arrange
  std::vector<base::test::ScopedFeatureList::FeatureAndParams> enabled_features;
  base::FieldTrialParams kServingParameters;
  kServingParameters["maximum_new_tab_page_ads_per_hour"] = "42";
  enabled_features.push_back({kServing, kServingParameters});

  const std::vector<base::Feature> disabled_features;

  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitWithFeaturesAndParameters(enabled_features,
                                                    disabled_features);

  // Act
  const int maximum_new_tab_page_ads_per_hour =
      GetMaximumNewTabPageAdsPerHour();

  // Assert
  const int expected_maximum_new_tab_page_ads_per_hour = 42;
  EXPECT_EQ(expected_maximum_new_tab_page_ads_per_hour,
            maximum_new_tab_page_ads_per_hour);
}

TEST(BatAdsServingFeaturesTest, DefaultMaximumNewTabPageAdsPerHour) {
  // Arrange
  const std::vector<base::test::ScopedFeatureList::FeatureAndParams>
      enabled_features;

  const std::vector<base::Feature> disabled_features;

  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitWithFeaturesAndParameters(enabled_features,
                                                    disabled_features);
  // Act
  const int maximum_new_tab_page_ads_per_hour =
      GetMaximumNewTabPageAdsPerHour();

  // Assert
  const int expected_maximum_new_tab_page_ads_per_hour = 4;
  EXPECT_EQ(expected_maximum_new_tab_page_ads_per_hour,
            maximum_new_tab_page_ads_per_hour);
}

TEST(BatAdsServingFeaturesTest, DisabledMaximumNewTabPageAdsPerHour) {
  // Arrange
  const std::vector<base::test::ScopedFeatureList::FeatureAndParams>
      enabled_features;

  std::vector<base::Feature> disabled_features;
  disabled_features.push_back(kServing);

  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitWithFeaturesAndParameters(enabled_features,
                                                    disabled_features);

  // Act
  const int maximum_new_tab_page_ads_per_hour =
      GetMaximumNewTabPageAdsPerHour();

  // Assert
  const int expected_maximum_new_tab_page_ads_per_hour = 4;
  EXPECT_EQ(expected_maximum_new_tab_page_ads_per_hour,
            maximum_new_tab_page_ads_per_hour);
}

TEST(BatAdsServingFeaturesTest, MaximumNewTabPageAdsPerDay) {
  // Arrange
  std::vector<base::test::ScopedFeatureList::FeatureAndParams> enabled_features;
  base::FieldTrialParams kServingParameters;
  kServingParameters["maximum_new_tab_page_ads_per_day"] = "24";
  enabled_features.push_back({kServing, kServingParameters});

  const std::vector<base::Feature> disabled_features;

  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitWithFeaturesAndParameters(enabled_features,
                                                    disabled_features);

  // Act
  const int maximum_new_tab_page_ads_per_day = GetMaximumNewTabPageAdsPerDay();

  // Assert
  const int expected_maximum_new_tab_page_ads_per_day = 24;
  EXPECT_EQ(expected_maximum_new_tab_page_ads_per_day,
            maximum_new_tab_page_ads_per_day);
}

TEST(BatAdsServingFeaturesTest, DefaultMaximumNewTabPageAdsPerDay) {
  // Arrange
  const std::vector<base::test::ScopedFeatureList::FeatureAndParams>
      enabled_features;

  const std::vector<base::Feature> disabled_features;

  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitWithFeaturesAndParameters(enabled_features,
                                                    disabled_features);
  // Act
  const int maximum_new_tab_page_ads_per_day = GetMaximumNewTabPageAdsPerDay();

  // Assert
  const int expected_maximum_new_tab_page_ads_per_day = 20;
  EXPECT_EQ(expected_maximum_new_tab_page_ads_per_day,
            maximum_new_tab_page_ads_per_day);
}

TEST(BatAdsServingFeaturesTest, DisabledMaximumNewTabPageAdsPerDay) {
  // Arrange
  const std::vector<base::test::ScopedFeatureList::FeatureAndParams>
      enabled_features;

  std::vector<base::Feature> disabled_features;
  disabled_features.push_back(kServing);

  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitWithFeaturesAndParameters(enabled_features,
                                                    disabled_features);

  // Act
  const int maximum_new_tab_page_ads_per_day = GetMaximumNewTabPageAdsPerDay();

  // Assert
  const int expected_maximum_new_tab_page_ads_per_day = 20;
  EXPECT_EQ(expected_maximum_new_tab_page_ads_per_day,
            maximum_new_tab_page_ads_per_day);
}

TEST(BatAdsServingFeaturesTest, MaximumPromotedContentAdsPerHour) {
  // Arrange
  std::vector<base::test::ScopedFeatureList::FeatureAndParams> enabled_features;
  base::FieldTrialParams kServingParameters;
  kServingParameters["maximum_promoted_content_ads_per_hour"] = "21";
  enabled_features.push_back({kServing, kServingParameters});

  const std::vector<base::Feature> disabled_features;

  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitWithFeaturesAndParameters(enabled_features,
                                                    disabled_features);

  // Act
  const int maximum_promoted_content_ads_per_hour =
      GetMaximumPromotedContentAdsPerHour();

  // Assert
  const int expected_maximum_promoted_content_ads_per_hour = 21;
  EXPECT_EQ(expected_maximum_promoted_content_ads_per_hour,
            maximum_promoted_content_ads_per_hour);
}

TEST(BatAdsServingFeaturesTest, DefaultMaximumPromotedContentAdsPerHour) {
  // Arrange
  const std::vector<base::test::ScopedFeatureList::FeatureAndParams>
      enabled_features;

  const std::vector<base::Feature> disabled_features;

  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitWithFeaturesAndParameters(enabled_features,
                                                    disabled_features);
  // Act
  const int maximum_promoted_content_ads_per_hour =
      GetMaximumPromotedContentAdsPerHour();

  // Assert
  const int expected_maximum_promoted_content_ads_per_hour = 4;
  EXPECT_EQ(expected_maximum_promoted_content_ads_per_hour,
            maximum_promoted_content_ads_per_hour);
}

TEST(BatAdsServingFeaturesTest, DisabledMaximumPromotedContentAdsPerHour) {
  // Arrange
  const std::vector<base::test::ScopedFeatureList::FeatureAndParams>
      enabled_features;

  std::vector<base::Feature> disabled_features;
  disabled_features.push_back(kServing);

  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitWithFeaturesAndParameters(enabled_features,
                                                    disabled_features);

  // Act
  const int maximum_promoted_content_ads_per_hour =
      GetMaximumPromotedContentAdsPerHour();

  // Assert
  const int expected_maximum_promoted_content_ads_per_hour = 4;
  EXPECT_EQ(expected_maximum_promoted_content_ads_per_hour,
            maximum_promoted_content_ads_per_hour);
}

TEST(BatAdsServingFeaturesTest, MaximumPromotedContentAdsPerDay) {
  // Arrange
  std::vector<base::test::ScopedFeatureList::FeatureAndParams> enabled_features;
  base::FieldTrialParams kServingParameters;
  kServingParameters["maximum_promoted_content_ads_per_day"] = "24";
  enabled_features.push_back({kServing, kServingParameters});

  const std::vector<base::Feature> disabled_features;

  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitWithFeaturesAndParameters(enabled_features,
                                                    disabled_features);

  // Act
  const int maximum_promoted_content_ads_per_day =
      GetMaximumPromotedContentAdsPerDay();

  // Assert
  const int expected_maximum_promoted_content_ads_per_day = 24;
  EXPECT_EQ(expected_maximum_promoted_content_ads_per_day,
            maximum_promoted_content_ads_per_day);
}

TEST(BatAdsServingFeaturesTest, DefaultMaximumPromotedContentAdsPerDay) {
  // Arrange
  const std::vector<base::test::ScopedFeatureList::FeatureAndParams>
      enabled_features;

  const std::vector<base::Feature> disabled_features;

  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitWithFeaturesAndParameters(enabled_features,
                                                    disabled_features);
  // Act
  const int maximum_promoted_content_ads_per_day =
      GetMaximumPromotedContentAdsPerDay();

  // Assert
  const int expected_maximum_promoted_content_ads_per_day = 20;
  EXPECT_EQ(expected_maximum_promoted_content_ads_per_day,
            maximum_promoted_content_ads_per_day);
}

TEST(BatAdsServingFeaturesTest, DisabledMaximumPromotedContentAdsPerDay) {
  // Arrange
  const std::vector<base::test::ScopedFeatureList::FeatureAndParams>
      enabled_features;

  std::vector<base::Feature> disabled_features;
  disabled_features.push_back(kServing);

  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitWithFeaturesAndParameters(enabled_features,
                                                    disabled_features);

  // Act
  const int maximum_promoted_content_ads_per_day =
      GetMaximumPromotedContentAdsPerDay();

  // Assert
  const int expected_maximum_promoted_content_ads_per_day = 20;
  EXPECT_EQ(expected_maximum_promoted_content_ads_per_day,
            maximum_promoted_content_ads_per_day);
}

TEST(BatAdsServingFeaturesTest, MaximumSearchResultAdsPerHour) {
  // Arrange
  std::vector<base::test::ScopedFeatureList::FeatureAndParams> enabled_features;
  base::FieldTrialParams kServingParameters;
  kServingParameters["maximum_search_result_ads_per_hour"] = "21";
  enabled_features.push_back({kServing, kServingParameters});

  const std::vector<base::Feature> disabled_features;

  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitWithFeaturesAndParameters(enabled_features,
                                                    disabled_features);

  // Act
  const int maximum_search_result_ads_per_hour =
      GetMaximumSearchResultAdsPerHour();

  // Assert
  const int expected_maximum_search_result_ads_per_hour = 21;
  EXPECT_EQ(expected_maximum_search_result_ads_per_hour,
            maximum_search_result_ads_per_hour);
}

TEST(BatAdsServingFeaturesTest, DefaultMaximumSearchResultAdsPerHour) {
  // Arrange
  const std::vector<base::test::ScopedFeatureList::FeatureAndParams>
      enabled_features;

  const std::vector<base::Feature> disabled_features;

  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitWithFeaturesAndParameters(enabled_features,
                                                    disabled_features);
  // Act
  const int maximum_search_result_ads_per_hour =
      GetMaximumSearchResultAdsPerHour();

  // Assert
  const int expected_maximum_search_result_ads_per_hour = 10;
  EXPECT_EQ(expected_maximum_search_result_ads_per_hour,
            maximum_search_result_ads_per_hour);
}

TEST(BatAdsServingFeaturesTest, DisabledMaximumSearchResultAdsPerHour) {
  // Arrange
  const std::vector<base::test::ScopedFeatureList::FeatureAndParams>
      enabled_features;

  std::vector<base::Feature> disabled_features;
  disabled_features.push_back(kServing);

  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitWithFeaturesAndParameters(enabled_features,
                                                    disabled_features);

  // Act
  const int maximum_search_result_ads_per_hour =
      GetMaximumSearchResultAdsPerHour();

  // Assert
  const int expected_maximum_search_result_ads_per_hour = 10;
  EXPECT_EQ(expected_maximum_search_result_ads_per_hour,
            maximum_search_result_ads_per_hour);
}

TEST(BatAdsServingFeaturesTest, MaximumSearchResultAdsPerDay) {
  // Arrange
  std::vector<base::test::ScopedFeatureList::FeatureAndParams> enabled_features;
  base::FieldTrialParams kServingParameters;
  kServingParameters["maximum_search_result_ads_per_day"] = "24";
  enabled_features.push_back({kServing, kServingParameters});

  const std::vector<base::Feature> disabled_features;

  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitWithFeaturesAndParameters(enabled_features,
                                                    disabled_features);

  // Act
  const int maximum_search_result_ads_per_day =
      GetMaximumSearchResultAdsPerDay();

  // Assert
  const int expected_maximum_search_result_ads_per_day = 24;
  EXPECT_EQ(expected_maximum_search_result_ads_per_day,
            maximum_search_result_ads_per_day);
}

TEST(BatAdsServingFeaturesTest, DefaultMaximumSearchResultAdsPerDay) {
  // Arrange
  const std::vector<base::test::ScopedFeatureList::FeatureAndParams>
      enabled_features;

  const std::vector<base::Feature> disabled_features;

  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitWithFeaturesAndParameters(enabled_features,
                                                    disabled_features);
  // Act
  const int maximum_search_result_ads_per_day =
      GetMaximumSearchResultAdsPerDay();

  // Assert
  const int expected_maximum_search_result_ads_per_day = 40;
  EXPECT_EQ(expected_maximum_search_result_ads_per_day,
            maximum_search_result_ads_per_day);
}

TEST(BatAdsServingFeaturesTest, DisabledMaximumSearchResultAdsPerDay) {
  // Arrange
  const std::vector<base::test::ScopedFeatureList::FeatureAndParams>
      enabled_features;

  std::vector<base::Feature> disabled_features;
  disabled_features.push_back(kServing);

  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitWithFeaturesAndParameters(enabled_features,
                                                    disabled_features);

  // Act
  const int maximum_search_result_ads_per_day =
      GetMaximumSearchResultAdsPerDay();

  // Assert
  const int expected_maximum_search_result_ads_per_day = 40;
  EXPECT_EQ(expected_maximum_search_result_ads_per_day,
            maximum_search_result_ads_per_day);
}

TEST(BatAdsServingFeaturesTest, DefaultServingVersion) {
  // Arrange

  // Act
  const int serving_version = GetServingVersion();

  // Assert
  const int expected_serving_version = 1;
  EXPECT_EQ(expected_serving_version, serving_version);
}

}  // namespace features
}  // namespace ads
