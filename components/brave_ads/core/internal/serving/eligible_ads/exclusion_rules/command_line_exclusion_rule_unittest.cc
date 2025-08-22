/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/serving/eligible_ads/exclusion_rules/command_line_exclusion_rule.h"

#include "brave/components/brave_ads/core/internal/ad_units/ad_test_constants.h"
#include "brave/components/brave_ads/core/internal/common/test/test_base.h"
#include "brave/components/brave_ads/core/internal/creatives/creative_ad_info.h"
#include "brave/components/brave_ads/core/internal/global_state/global_state.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

class BraveAdsCommandLineExclusionRuleTest : public test::TestBase {
 protected:
  const CommandLineExclusionRule exclusion_rule_;
};

TEST_F(BraveAdsCommandLineExclusionRuleTest, ShouldIncludeCreativeInstance) {
  // Arrange
  GlobalState::GetInstance()->Flags().ads_uuids = {
      {test::kCreativeInstanceId, true},
  };

  CreativeAdInfo creative_ad;
  creative_ad.creative_instance_id = test::kCreativeInstanceId;

  // Act & Assert
  EXPECT_TRUE(exclusion_rule_.ShouldInclude(creative_ad));
}

TEST_F(BraveAdsCommandLineExclusionRuleTest, ShouldExcludeCreativeInstance) {
  // Arrange
  GlobalState::GetInstance()->Flags().ads_uuids = {
      {test::kAnotherCreativeInstanceId, true},
  };

  CreativeAdInfo creative_ad;
  creative_ad.creative_instance_id = test::kCreativeInstanceId;

  // Act & Assert
  EXPECT_FALSE(exclusion_rule_.ShouldInclude(creative_ad));
}

TEST_F(BraveAdsCommandLineExclusionRuleTest, ShouldIncludeCreativeSet) {
  // Arrange
  GlobalState::GetInstance()->Flags().ads_uuids = {
      {test::kCreativeSetId, true},
  };

  CreativeAdInfo creative_ad;
  creative_ad.creative_set_id = test::kCreativeSetId;

  // Act & Assert
  EXPECT_TRUE(exclusion_rule_.ShouldInclude(creative_ad));
}

TEST_F(BraveAdsCommandLineExclusionRuleTest, ShouldExcludeCreativeSet) {
  // Arrange
  GlobalState::GetInstance()->Flags().ads_uuids = {
      {test::kAnotherCreativeSetId, true},
  };

  CreativeAdInfo creative_ad;
  creative_ad.creative_set_id = test::kCreativeSetId;

  // Act & Assert
  EXPECT_FALSE(exclusion_rule_.ShouldInclude(creative_ad));
}

TEST_F(BraveAdsCommandLineExclusionRuleTest, ShouldIncludeCampaign) {
  // Arrange
  GlobalState::GetInstance()->Flags().ads_uuids = {
      {test::kCampaignId, true},
  };

  CreativeAdInfo creative_ad;
  creative_ad.campaign_id = test::kCampaignId;

  // Act & Assert
  EXPECT_TRUE(exclusion_rule_.ShouldInclude(creative_ad));
}

TEST_F(BraveAdsCommandLineExclusionRuleTest, ShouldExcludeCampaign) {
  // Arrange
  GlobalState::GetInstance()->Flags().ads_uuids = {
      {test::kAnotherCampaignId, true},
  };

  CreativeAdInfo creative_ad;
  creative_ad.campaign_id = test::kCampaignId;

  // Act & Assert
  EXPECT_FALSE(exclusion_rule_.ShouldInclude(creative_ad));
}

TEST_F(BraveAdsCommandLineExclusionRuleTest, ShouldIncludeAdvertiser) {
  // Arrange
  GlobalState::GetInstance()->Flags().ads_uuids = {
      {test::kAdvertiserId, true},
  };

  CreativeAdInfo creative_ad;
  creative_ad.advertiser_id = test::kAdvertiserId;

  // Act & Assert
  EXPECT_TRUE(exclusion_rule_.ShouldInclude(creative_ad));
}

TEST_F(BraveAdsCommandLineExclusionRuleTest, ShouldExcludeAdvertiser) {
  // Arrange
  GlobalState::GetInstance()->Flags().ads_uuids = {
      {test::kAnotherAdvertiserId, true},
  };

  CreativeAdInfo creative_ad;
  creative_ad.advertiser_id = test::kAdvertiserId;

  // Act & Assert
  EXPECT_FALSE(exclusion_rule_.ShouldInclude(creative_ad));
}

}  // namespace brave_ads
