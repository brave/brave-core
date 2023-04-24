/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/ads/serving/eligible_ads/exclusion_rules/anti_targeting_exclusion_rule.h"

#include "brave/components/brave_ads/core/internal/ads/ad_unittest_constants.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_base.h"
#include "brave/components/brave_ads/core/internal/creatives/creative_ad_info.h"
#include "brave/components/brave_ads/core/internal/resources/behavioral/anti_targeting/anti_targeting_resource.h"
#include "url/gurl.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

class BraveAdsAntiTargetingExclusionRuleTest : public UnitTestBase {};

TEST_F(BraveAdsAntiTargetingExclusionRuleTest,
       AllowIfResourceIsNotInitialized) {
  // Arrange
  CreativeAdInfo creative_ad;
  creative_ad.creative_set_id = kCreativeSetId;

  resource::AntiTargeting resource;

  const BrowsingHistoryList history = {GURL("https://www.foo1.org"),
                                       GURL("https://www.brave.com"),
                                       GURL("https://www.foo2.org")};

  AntiTargetingExclusionRule exclusion_rule(resource, history);

  // Act

  // Assert
  EXPECT_TRUE(exclusion_rule.ShouldInclude(creative_ad).has_value());
}

TEST_F(BraveAdsAntiTargetingExclusionRuleTest, AllowIfCreativeSetDoesNotExist) {
  // Arrange
  CreativeAdInfo creative_ad;
  creative_ad.creative_set_id = kMissingCreativeSetId;

  resource::AntiTargeting resource;
  resource.Load();
  task_environment_.RunUntilIdle();

  const BrowsingHistoryList history = {GURL("https://www.foo1.org"),
                                       GURL("https://www.brave.com"),
                                       GURL("https://www.foo2.org")};

  AntiTargetingExclusionRule exclusion_rule(resource, history);

  // Act

  // Assert
  EXPECT_TRUE(exclusion_rule.ShouldInclude(creative_ad).has_value());
}

TEST_F(BraveAdsAntiTargetingExclusionRuleTest, AllowIfSiteDoesNotExist) {
  // Arrange
  CreativeAdInfo creative_ad;
  creative_ad.creative_set_id = kCreativeSetId;

  resource::AntiTargeting resource;
  resource.Load();
  task_environment_.RunUntilIdle();

  const BrowsingHistoryList history = {GURL("https://www.foo1.org"),
                                       GURL("https://www.foo2.org")};

  AntiTargetingExclusionRule exclusion_rule(resource, history);

  // Act

  // Assert
  EXPECT_TRUE(exclusion_rule.ShouldInclude(creative_ad).has_value());
}

TEST_F(BraveAdsAntiTargetingExclusionRuleTest,
       DoNotAllowIfCreativeSetAndSiteMatch) {
  // Arrange
  CreativeAdInfo creative_ad;
  creative_ad.creative_set_id = kCreativeSetId;

  resource::AntiTargeting resource;
  resource.Load();
  task_environment_.RunUntilIdle();

  const BrowsingHistoryList history = {GURL("https://www.foo1.org"),
                                       GURL("https://www.brave.com")};

  AntiTargetingExclusionRule exclusion_rule(resource, history);

  // Act

  // Assert
  EXPECT_FALSE(exclusion_rule.ShouldInclude(creative_ad).has_value());
}

}  // namespace brave_ads
