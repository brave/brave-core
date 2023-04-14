/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/ads/serving/permission_rules/media_permission_rule.h"

#include <vector>

#include "base/test/scoped_feature_list.h"
#include "brave/components/brave_ads/core/internal/ads/serving/permission_rules/permission_rule_features.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_base.h"
#include "url/gurl.h"

// npm run test -- brave_unit_tests --filter=BatAds*

namespace brave_ads {

class BatAdsMediaPermissionRuleTest : public UnitTestBase {
 protected:
  MediaPermissionRule permission_rule_;
};

TEST_F(BatAdsMediaPermissionRuleTest, AllowAdIfMediaIsNotPlaying) {
  // Arrange

  // Act

  // Assert
  EXPECT_TRUE(permission_rule_.ShouldAllow());
}

TEST_F(BatAdsMediaPermissionRuleTest, AllowAdIfMediaIsStoppedForSingleTab) {
  // Arrange
  NotifyTabDidChange(
      /*id*/ 1, /*redirect_chain*/ {GURL("https://brave.com")},
      /*is_active*/ true,
      /*is_incognito*/ false);

  NotifyTabDidStartPlayingMedia(/*id*/ 1);

  // Act
  NotifyTabDidStopPlayingMedia(/*id*/ 1);

  // Assert
  EXPECT_TRUE(permission_rule_.ShouldAllow());
}

TEST_F(BatAdsMediaPermissionRuleTest, AllowAdIfMediaIsStoppedOnMultipleTabs) {
  // Arrange
  NotifyTabDidChange(
      /*id*/ 1, /*redirect_chain*/ {GURL("https://brave.com")},
      /*is_active*/ true,
      /*is_incognito*/ false);

  NotifyTabDidStartPlayingMedia(/*id*/ 1);
  NotifyTabDidStartPlayingMedia(/*id*/ 2);

  // Act
  NotifyTabDidStopPlayingMedia(/*id*/ 1);
  NotifyTabDidStopPlayingMedia(/*id*/ 2);

  // Assert
  EXPECT_TRUE(permission_rule_.ShouldAllow());
}

TEST_F(BatAdsMediaPermissionRuleTest,
       AllowAdIfMediaIsPlayingOnMultipleTabsButStoppedForVisibleTab) {
  // Arrange
  NotifyTabDidChange(
      /*id*/ 1, /*redirect_chain*/ {GURL("https://brave.com")},
      /*is_active*/ true,
      /*is_incognito*/ false);

  NotifyTabDidStartPlayingMedia(/*id*/ 1);
  NotifyTabDidStartPlayingMedia(/*id*/ 2);

  // Act
  NotifyTabDidStopPlayingMedia(/*id*/ 1);

  // Assert
  EXPECT_TRUE(permission_rule_.ShouldAllow());
}

TEST_F(BatAdsMediaPermissionRuleTest,
       DoNotAllowAdIfMediaIsPlayingOnVisibleTab) {
  // Arrange
  NotifyTabDidChange(
      /*id*/ 1, /*redirect_chain*/ {GURL("https://brave.com")},
      /*is_active*/ true,
      /*is_incognito*/ false);

  // Act
  NotifyTabDidStartPlayingMedia(/*id*/ 1);

  // Assert
  EXPECT_FALSE(permission_rule_.ShouldAllow());
}

TEST_F(BatAdsMediaPermissionRuleTest,
       AlwaysAllowAdIfMediaIsPlayingOnVisibleTabIfPermissionRuleIsDisabled) {
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
      /*is_active*/ true,
      /*is_incognito*/ false);

  NotifyTabDidStartPlayingMedia(/*id*/ 1);

  // Act

  // Assert
  EXPECT_TRUE(permission_rule_.ShouldAllow());
}

TEST_F(BatAdsMediaPermissionRuleTest,
       DoNotAllowAdIfMediaIsPlayingOnMultipleTabs) {
  // Arrange
  NotifyTabDidChange(
      /*id*/ 1, /*redirect_chain*/ {GURL("https://brave.com")},
      /*is_active*/ true,
      /*is_incognito*/ false);

  // Act
  NotifyTabDidStartPlayingMedia(/*id*/ 1);
  NotifyTabDidStartPlayingMedia(/*id*/ 2);

  // Assert
  EXPECT_FALSE(permission_rule_.ShouldAllow());
}

TEST_F(BatAdsMediaPermissionRuleTest,
       DoNotAllowAdIfMediaIsPlayingOnMultipleTabsButStoppedForOccludedTab) {
  // Arrange
  NotifyTabDidChange(
      /*id*/ 1, /*redirect_chain*/ {GURL("https://brave.com")},
      /*is_active*/ true,
      /*is_incognito*/ false);

  NotifyTabDidStartPlayingMedia(/*id*/ 1);
  NotifyTabDidStartPlayingMedia(/*id*/ 2);

  // Act
  NotifyTabDidStopPlayingMedia(/*id*/ 2);

  // Assert
  EXPECT_FALSE(permission_rule_.ShouldAllow());
}

}  // namespace brave_ads
