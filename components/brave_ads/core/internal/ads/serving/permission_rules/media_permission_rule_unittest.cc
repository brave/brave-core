/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/ads/serving/permission_rules/media_permission_rule.h"

#include <vector>

#include "base/test/scoped_feature_list.h"
#include "brave/components/brave_ads/core/internal/ads/serving/permission_rules/permission_rule_feature.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_base.h"
#include "url/gurl.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

class BraveAdsMediaPermissionRuleTest : public UnitTestBase {
 protected:
  const MediaPermissionRule permission_rule_;
};

TEST_F(BraveAdsMediaPermissionRuleTest, ShouldAllowIfMediaIsNotPlaying) {
  // Arrange

  // Act

  // Assert
  EXPECT_TRUE(permission_rule_.ShouldAllow().has_value());
}

TEST_F(BraveAdsMediaPermissionRuleTest,
       ShouldAllowIfMediaIsStoppedForSingleTab) {
  // Arrange
  NotifyTabDidChange(
      /*id*/ 1, /*redirect_chain*/ {GURL("https://brave.com")},
      /*is_active*/ true);

  NotifyTabDidStartPlayingMedia(/*id*/ 1);

  // Act
  NotifyTabDidStopPlayingMedia(/*id*/ 1);

  // Assert
  EXPECT_TRUE(permission_rule_.ShouldAllow().has_value());
}

TEST_F(BraveAdsMediaPermissionRuleTest,
       ShouldAllowIfMediaIsStoppedOnMultipleTabs) {
  // Arrange
  NotifyTabDidChange(
      /*id*/ 1, /*redirect_chain*/ {GURL("https://brave.com")},
      /*is_active*/ true);

  NotifyTabDidStartPlayingMedia(/*id*/ 1);
  NotifyTabDidStartPlayingMedia(/*id*/ 2);

  // Act
  NotifyTabDidStopPlayingMedia(/*id*/ 1);
  NotifyTabDidStopPlayingMedia(/*id*/ 2);

  // Assert
  EXPECT_TRUE(permission_rule_.ShouldAllow().has_value());
}

TEST_F(BraveAdsMediaPermissionRuleTest,
       ShouldAllowIfMediaIsPlayingOnMultipleTabsButStoppedForVisibleTab) {
  // Arrange
  NotifyTabDidChange(
      /*id*/ 1, /*redirect_chain*/ {GURL("https://brave.com")},
      /*is_active*/ true);

  NotifyTabDidStartPlayingMedia(/*id*/ 1);
  NotifyTabDidStartPlayingMedia(/*id*/ 2);

  // Act
  NotifyTabDidStopPlayingMedia(/*id*/ 1);

  // Assert
  EXPECT_TRUE(permission_rule_.ShouldAllow().has_value());
}

TEST_F(BraveAdsMediaPermissionRuleTest,
       ShouldNotAllowIfMediaIsPlayingOnVisibleTab) {
  // Arrange
  NotifyTabDidChange(
      /*id*/ 1, /*redirect_chain*/ {GURL("https://brave.com")},
      /*is_active*/ true);

  // Act
  NotifyTabDidStartPlayingMedia(/*id*/ 1);

  // Assert
  EXPECT_FALSE(permission_rule_.ShouldAllow().has_value());
}

TEST_F(
    BraveAdsMediaPermissionRuleTest,
    ShouldAlwaysAllowIfMediaIsPlayingOnVisibleTabIfPermissionRuleIsDisabled) {
  // Arrange
  base::FieldTrialParams params;
  params["should_only_serve_ads_if_media_is_not_playing"] = "false";
  std::vector<base::test::FeatureRefAndParams> enabled_features;
  enabled_features.emplace_back(kPermissionRulesFeature, params);

  const std::vector<base::test::FeatureRef> disabled_features;

  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitWithFeaturesAndParameters(enabled_features,
                                                    disabled_features);

  NotifyTabDidChange(
      /*id*/ 1, /*redirect_chain*/ {GURL("https://brave.com")},
      /*is_active*/ true);

  NotifyTabDidStartPlayingMedia(/*id*/ 1);

  // Act

  // Assert
  EXPECT_TRUE(permission_rule_.ShouldAllow().has_value());
}

TEST_F(BraveAdsMediaPermissionRuleTest,
       ShouldNotAllowIfMediaIsPlayingOnMultipleTabs) {
  // Arrange
  NotifyTabDidChange(
      /*id*/ 1, /*redirect_chain*/ {GURL("https://brave.com")},
      /*is_active*/ true);

  // Act
  NotifyTabDidStartPlayingMedia(/*id*/ 1);
  NotifyTabDidStartPlayingMedia(/*id*/ 2);

  // Assert
  EXPECT_FALSE(permission_rule_.ShouldAllow().has_value());
}

TEST_F(BraveAdsMediaPermissionRuleTest,
       ShouldNotAllowIfMediaIsPlayingOnMultipleTabsButStoppedForOccludedTab) {
  // Arrange
  NotifyTabDidChange(
      /*id*/ 1, /*redirect_chain*/ {GURL("https://brave.com")},
      /*is_active*/ true);

  NotifyTabDidStartPlayingMedia(/*id*/ 1);
  NotifyTabDidStartPlayingMedia(/*id*/ 2);

  // Act
  NotifyTabDidStopPlayingMedia(/*id*/ 2);

  // Assert
  EXPECT_FALSE(permission_rule_.ShouldAllow().has_value());
}

}  // namespace brave_ads
