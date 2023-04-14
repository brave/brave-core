/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/ads/serving/eligible_ads/exclusion_rules/notification_ads/notification_ad_embedding_exclusion_rule.h"

#include <vector>

#include "base/test/scoped_feature_list.h"
#include "brave/components/brave_ads/core/internal/ads/serving/notification_ad_serving_features.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_base.h"

// npm run test -- brave_unit_tests --filter=BatAds*

namespace brave_ads::notification_ads {

namespace {
constexpr char kCreativeSetId[] = "654f10df-fbc4-4a92-8d43-2edf73734a60";
}  // namespace

class EmbeddingExclusionRuleTest : public UnitTestBase {};

TEST_F(EmbeddingExclusionRuleTest, AllowIfNoEmbeddingV2) {
  // Arrange
  CreativeAdInfo creative_ad;
  creative_ad.creative_set_id = kCreativeSetId;

  std::vector<base::test::FeatureRefAndParams> enabled_features;
  base::FieldTrialParams params;
  params["version"] = "2";
  enabled_features.emplace_back(notification_ads::kServingFeature, params);

  const std::vector<base::test::FeatureRef> disabled_features;

  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitWithFeaturesAndParameters(enabled_features,
                                                    disabled_features);

  // Act
  EmbeddingExclusionRule exclusion_rule;
  const bool should_exclude = exclusion_rule.ShouldExclude(creative_ad);

  // Assert
  EXPECT_FALSE(should_exclude);
}

TEST_F(EmbeddingExclusionRuleTest, AllowIfEmbeddingV2) {
  // Arrange
  CreativeAdInfo creative_ad;
  creative_ad.creative_set_id = kCreativeSetId;
  creative_ad.embedding = {1.0, 2.0, 3.0, 4.0, 5.0};

  std::vector<base::test::FeatureRefAndParams> enabled_features;
  base::FieldTrialParams params;
  params["version"] = "2";
  enabled_features.emplace_back(notification_ads::kServingFeature, params);

  const std::vector<base::test::FeatureRef> disabled_features;

  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitWithFeaturesAndParameters(enabled_features,
                                                    disabled_features);

  // Act
  EmbeddingExclusionRule exclusion_rule;
  const bool should_exclude = exclusion_rule.ShouldExclude(creative_ad);

  // Assert
  EXPECT_FALSE(should_exclude);
}

TEST_F(EmbeddingExclusionRuleTest, DisallowIfNoEmbeddingV3) {
  // Arrange
  CreativeAdInfo creative_ad;
  creative_ad.creative_set_id = kCreativeSetId;

  std::vector<base::test::FeatureRefAndParams> enabled_features;
  base::FieldTrialParams params;
  params["version"] = "3";
  enabled_features.emplace_back(notification_ads::kServingFeature, params);

  const std::vector<base::test::FeatureRef> disabled_features;

  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitWithFeaturesAndParameters(enabled_features,
                                                    disabled_features);

  // Act
  EmbeddingExclusionRule exclusion_rule;
  const bool should_exclude = exclusion_rule.ShouldExclude(creative_ad);

  // Assert
  EXPECT_TRUE(should_exclude);
}

TEST_F(EmbeddingExclusionRuleTest, DisallowIfEmptyEmbeddingV3) {
  // Arrange
  CreativeAdInfo creative_ad;
  creative_ad.creative_set_id = kCreativeSetId;
  creative_ad.embedding = {};

  std::vector<base::test::FeatureRefAndParams> enabled_features;
  base::FieldTrialParams params;
  params["version"] = "3";
  enabled_features.emplace_back(notification_ads::kServingFeature, params);

  const std::vector<base::test::FeatureRef> disabled_features;

  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitWithFeaturesAndParameters(enabled_features,
                                                    disabled_features);

  // Act
  EmbeddingExclusionRule exclusion_rule;
  const bool should_exclude = exclusion_rule.ShouldExclude(creative_ad);

  // Assert
  EXPECT_TRUE(should_exclude);
}

TEST_F(EmbeddingExclusionRuleTest, AllowIfEmbeddingV3) {
  // Arrange
  CreativeAdInfo creative_ad;
  creative_ad.creative_set_id = kCreativeSetId;
  creative_ad.embedding = {1.0, 2.0, 3.0, 4.0, 5.0};

  std::vector<base::test::FeatureRefAndParams> enabled_features;
  base::FieldTrialParams params;
  params["version"] = "3";
  enabled_features.emplace_back(notification_ads::kServingFeature, params);

  const std::vector<base::test::FeatureRef> disabled_features;

  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitWithFeaturesAndParameters(enabled_features,
                                                    disabled_features);

  // Act
  EmbeddingExclusionRule exclusion_rule;
  const bool should_exclude = exclusion_rule.ShouldExclude(creative_ad);

  // Assert
  EXPECT_FALSE(should_exclude);
}

}  // namespace brave_ads::notification_ads
