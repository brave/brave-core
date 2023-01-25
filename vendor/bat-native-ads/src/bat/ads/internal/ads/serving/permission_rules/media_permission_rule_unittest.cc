/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/ads/serving/permission_rules/media_permission_rule.h"

#include <vector>

#include "base/test/scoped_feature_list.h"
#include "bat/ads/internal/ads/serving/permission_rules/permission_rule_features.h"
#include "bat/ads/internal/common/unittest/unittest_base.h"
#include "url/gurl.h"

// npm run test -- brave_unit_tests --filter=BatAds*

namespace ads {

class BatAdsMediaPermissionRuleTest : public UnitTestBase {};

TEST_F(BatAdsMediaPermissionRuleTest, AllowAdIfMediaIsNotPlaying) {
  // Arrange

  // Act
  MediaPermissionRule permission_rule;
  const bool is_allowed = permission_rule.ShouldAllow();

  // Assert
  EXPECT_TRUE(is_allowed);
}

TEST_F(BatAdsMediaPermissionRuleTest, AllowAdIfMediaIsStoppedForSingleTab) {
  // Arrange
  TabManager::GetInstance()->OnDidChange(
      /*id*/ 1, /*redirect_chain*/ {GURL("https://brave.com")},
      /*is_active*/ true,
      /*is_incognito*/ false);

  TabManager::GetInstance()->OnDidStartPlayingMedia(/*id*/ 1);
  TabManager::GetInstance()->OnDidStopPlayingMedia(/*id*/ 1);

  // Act
  MediaPermissionRule permission_rule;
  const bool is_allowed = permission_rule.ShouldAllow();

  // Assert
  EXPECT_TRUE(is_allowed);
}

TEST_F(BatAdsMediaPermissionRuleTest, AllowAdIfMediaIsStoppedOnMultipleTabs) {
  // Arrange
  TabManager::GetInstance()->OnDidChange(
      /*id*/ 1, /*redirect_chain*/ {GURL("https://brave.com")},
      /*is_active*/ true,
      /*is_incognito*/ false);

  TabManager::GetInstance()->OnDidStartPlayingMedia(/*id*/ 1);
  TabManager::GetInstance()->OnDidStartPlayingMedia(/*id*/ 2);
  TabManager::GetInstance()->OnDidStopPlayingMedia(/*id*/ 1);
  TabManager::GetInstance()->OnDidStopPlayingMedia(/*id*/ 2);

  // Act
  MediaPermissionRule permission_rule;
  const bool is_allowed = permission_rule.ShouldAllow();

  // Assert
  EXPECT_TRUE(is_allowed);
}

TEST_F(BatAdsMediaPermissionRuleTest,
       AllowAdIfMediaIsPlayingOnMultipleTabsButStoppedForVisibleTab) {
  // Arrange
  TabManager::GetInstance()->OnDidChange(
      /*id*/ 1, /*redirect_chain*/ {GURL("https://brave.com")},
      /*is_active*/ true,
      /*is_incognito*/ false);

  TabManager::GetInstance()->OnDidStartPlayingMedia(/*id*/ 1);
  TabManager::GetInstance()->OnDidStartPlayingMedia(/*id*/ 2);
  TabManager::GetInstance()->OnDidStopPlayingMedia(/*id*/ 1);

  // Act
  MediaPermissionRule permission_rule;
  const bool is_allowed = permission_rule.ShouldAllow();

  // Assert
  EXPECT_TRUE(is_allowed);
}

TEST_F(BatAdsMediaPermissionRuleTest,
       DoNotAllowAdIfMediaIsPlayingOnVisibleTab) {
  // Arrange
  TabManager::GetInstance()->OnDidChange(
      /*id*/ 1, /*redirect_chain*/ {GURL("https://brave.com")},
      /*is_active*/ true,
      /*is_incognito*/ false);

  TabManager::GetInstance()->OnDidStartPlayingMedia(/*id*/ 1);

  // Act
  MediaPermissionRule permission_rule;
  const bool is_allowed = permission_rule.ShouldAllow();

  // Assert
  EXPECT_FALSE(is_allowed);
}

TEST_F(BatAdsMediaPermissionRuleTest,
       AlwaysAllowAdIfMediaIsPlayingOnVisibleTabIfPermissionRuleIsDisabled) {
  // Arrange
  base::FieldTrialParams params;
  params["should_only_serve_ads_if_media_is_not_playing"] = "false";
  std::vector<base::test::FeatureRefAndParams> enabled_features;
  enabled_features.emplace_back(permission_rules::features::kFeature, params);

  const std::vector<base::test::FeatureRef> disabled_features;

  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitWithFeaturesAndParameters(enabled_features,
                                                    disabled_features);

  TabManager::GetInstance()->OnDidChange(
      /*id*/ 1, /*redirect_chain*/ {GURL("https://brave.com")},
      /*is_active*/ true,
      /*is_incognito*/ false);

  TabManager::GetInstance()->OnDidStartPlayingMedia(/*id*/ 1);

  // Act
  MediaPermissionRule permission_rule;
  const bool is_allowed = permission_rule.ShouldAllow();

  // Assert
  EXPECT_TRUE(is_allowed);
}

TEST_F(BatAdsMediaPermissionRuleTest,
       DoNotAllowAdIfMediaIsPlayingOnMultipleTabs) {
  // Arrange
  TabManager::GetInstance()->OnDidChange(
      /*id*/ 1, /*redirect_chain*/ {GURL("https://brave.com")},
      /*is_active*/ true,
      /*is_incognito*/ false);

  TabManager::GetInstance()->OnDidStartPlayingMedia(/*id*/ 1);
  TabManager::GetInstance()->OnDidStartPlayingMedia(/*id*/ 2);

  // Act
  MediaPermissionRule permission_rule;
  const bool is_allowed = permission_rule.ShouldAllow();

  // Assert
  EXPECT_FALSE(is_allowed);
}

TEST_F(BatAdsMediaPermissionRuleTest,
       DoNotAllowAdIfMediaIsPlayingOnMultipleTabsButStoppedForOccludedTab) {
  // Arrange
  TabManager::GetInstance()->OnDidChange(
      /*id*/ 1, /*redirect_chain*/ {GURL("https://brave.com")},
      /*is_active*/ true,
      /*is_incognito*/ false);

  TabManager::GetInstance()->OnDidStartPlayingMedia(/*id*/ 1);
  TabManager::GetInstance()->OnDidStartPlayingMedia(/*id*/ 2);
  TabManager::GetInstance()->OnDidStopPlayingMedia(/*id*/ 2);

  // Act
  MediaPermissionRule permission_rule;
  const bool is_allowed = permission_rule.ShouldAllow();

  // Assert
  EXPECT_FALSE(is_allowed);
}

}  // namespace ads
