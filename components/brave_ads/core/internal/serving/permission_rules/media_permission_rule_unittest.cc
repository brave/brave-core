/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/serving/permission_rules/media_permission_rule.h"

#include "base/test/scoped_feature_list.h"
#include "brave/components/brave_ads/core/internal/common/test/test_base.h"
#include "brave/components/brave_ads/core/internal/serving/permission_rules/permission_rule_feature.h"
#include "url/gurl.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

class BraveAdsMediaPermissionRuleTest : public test::TestBase {};

TEST_F(BraveAdsMediaPermissionRuleTest, ShouldAllowIfMediaIsNotPlaying) {
  // Act & Assert
  EXPECT_TRUE(HasMediaPermission());
}

TEST_F(BraveAdsMediaPermissionRuleTest,
       ShouldAllowIfMediaIsStoppedForSingleTab) {
  // Arrange
  SimulateOpeningNewTab(/*tab_id=*/1,
                        /*redirect_chain=*/{GURL("https://brave.com")});

  NotifyTabDidStartPlayingMedia(/*tab_id=*/1);

  NotifyTabDidStopPlayingMedia(/*tab_id=*/1);

  // Act & Assert
  EXPECT_TRUE(HasMediaPermission());
}

TEST_F(BraveAdsMediaPermissionRuleTest,
       ShouldAllowIfMediaIsStoppedOnMultipleTabs) {
  // Arrange
  SimulateOpeningNewTab(/*tab_id=*/1,
                        /*redirect_chain=*/{GURL("https://brave.com")});

  NotifyTabDidStartPlayingMedia(/*tab_id=*/1);
  NotifyTabDidStartPlayingMedia(/*tab_id=*/2);

  NotifyTabDidStopPlayingMedia(/*tab_id=*/1);
  NotifyTabDidStopPlayingMedia(/*tab_id=*/2);

  // Act & Assert
  EXPECT_TRUE(HasMediaPermission());
}

TEST_F(BraveAdsMediaPermissionRuleTest,
       ShouldAllowIfMediaIsPlayingOnMultipleTabsButStoppedForVisibleTab) {
  // Arrange
  SimulateOpeningNewTab(/*tab_id=*/1,
                        /*redirect_chain=*/{GURL("https://brave.com")});

  NotifyTabDidStartPlayingMedia(/*tab_id=*/1);
  NotifyTabDidStartPlayingMedia(/*tab_id=*/2);

  NotifyTabDidStopPlayingMedia(/*tab_id=*/1);

  // Act & Assert
  EXPECT_TRUE(HasMediaPermission());
}

TEST_F(BraveAdsMediaPermissionRuleTest,
       ShouldNotAllowIfMediaIsPlayingOnVisibleTab) {
  // Arrange
  SimulateOpeningNewTab(/*tab_id=*/1,
                        /*redirect_chain=*/{GURL("https://brave.com")});

  NotifyTabDidStartPlayingMedia(/*tab_id=*/1);

  // Act & Assert
  EXPECT_FALSE(HasMediaPermission());
}

TEST_F(
    BraveAdsMediaPermissionRuleTest,
    ShouldAlwaysAllowIfMediaIsPlayingOnVisibleTabIfPermissionRuleIsDisabled) {
  // Arrange
  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitAndEnableFeatureWithParameters(
      kPermissionRulesFeature,
      {{"should_only_serve_ads_if_media_is_not_playing", "false"}});

  SimulateOpeningNewTab(/*tab_id=*/1,
                        /*redirect_chain=*/{GURL("https://brave.com")});

  NotifyTabDidStartPlayingMedia(/*tab_id=*/1);

  // Act & Assert
  EXPECT_TRUE(HasMediaPermission());
}

TEST_F(BraveAdsMediaPermissionRuleTest,
       ShouldNotAllowIfMediaIsPlayingOnMultipleTabs) {
  // Arrange
  SimulateOpeningNewTab(/*tab_id=*/1,
                        /*redirect_chain=*/{GURL("https://brave.com")});

  NotifyTabDidStartPlayingMedia(/*tab_id=*/1);
  NotifyTabDidStartPlayingMedia(/*tab_id=*/2);

  // Act & Assert
  EXPECT_FALSE(HasMediaPermission());
}

TEST_F(BraveAdsMediaPermissionRuleTest,
       ShouldNotAllowIfMediaIsPlayingOnMultipleTabsButStoppedForOccludedTab) {
  // Arrange
  SimulateOpeningNewTab(/*tab_id=*/1,
                        /*redirect_chain=*/{GURL("https://brave.com")});

  NotifyTabDidStartPlayingMedia(/*tab_id=*/1);
  NotifyTabDidStartPlayingMedia(/*tab_id=*/2);

  NotifyTabDidStopPlayingMedia(/*tab_id=*/2);

  // Act & Assert
  EXPECT_FALSE(HasMediaPermission());
}

}  // namespace brave_ads
