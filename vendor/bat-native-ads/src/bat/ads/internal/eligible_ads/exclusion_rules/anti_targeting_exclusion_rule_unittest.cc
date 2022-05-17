/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/eligible_ads/exclusion_rules/anti_targeting_exclusion_rule.h"

#include "bat/ads/internal/base/unittest_base.h"
#include "bat/ads/internal/base/unittest_util.h"
#include "bat/ads/internal/resources/behavioral/anti_targeting/anti_targeting_resource.h"

// npm run test -- brave_unit_tests --filter=BatAds*

namespace ads {

namespace {

constexpr char kCreativeSetIdOnAntiTargetingList[] =
    "5bdeab83-048f-48a7-9602-a1092ded123c";
constexpr char kCreativeSetIdNotOnAntiTargetingList[] =
    "d175cdfd-57bf-46c3-9b00-89eed71c6ae5";

}  // namespace

class BatAdsAntiTargetingExclusionRuleTest : public UnitTestBase {
 protected:
  BatAdsAntiTargetingExclusionRuleTest() = default;

  ~BatAdsAntiTargetingExclusionRuleTest() override = default;
};

TEST_F(BatAdsAntiTargetingExclusionRuleTest, AllowIfResourceDidNotLoad) {
  // Arrange
  CreativeAdInfo creative_ad;
  creative_ad.creative_set_id = kCreativeSetIdOnAntiTargetingList;

  resource::AntiTargeting resource;

  BrowsingHistoryList history = {GURL("https://www.foo1.org"),
                                 GURL("https://www.brave.com"),
                                 GURL("https://www.foo2.org")};

  // Act
  AntiTargetingExclusionRule frequency_cap(&resource, history);
  const bool should_exclude = frequency_cap.ShouldExclude(creative_ad);

  // Assert
  EXPECT_FALSE(should_exclude);
}

TEST_F(BatAdsAntiTargetingExclusionRuleTest, AllowIfCreativeSetDoesNotMatch) {
  // Arrange
  CreativeAdInfo creative_ad;
  creative_ad.creative_set_id = kCreativeSetIdNotOnAntiTargetingList;

  resource::AntiTargeting resource;
  resource.Load();
  task_environment()->RunUntilIdle();

  BrowsingHistoryList history = {GURL("https://www.foo1.org"),
                                 GURL("https://www.brave.com"),
                                 GURL("https://www.foo2.org")};

  // Act
  AntiTargetingExclusionRule frequency_cap(&resource, history);
  const bool should_exclude = frequency_cap.ShouldExclude(creative_ad);

  // Assert
  EXPECT_FALSE(should_exclude);
}

TEST_F(BatAdsAntiTargetingExclusionRuleTest, AllowIfSiteDoesNotMatch) {
  // Arrange
  CreativeAdInfo creative_ad;
  creative_ad.creative_set_id = kCreativeSetIdOnAntiTargetingList;

  resource::AntiTargeting resource;
  resource.Load();
  task_environment()->RunUntilIdle();

  BrowsingHistoryList history = {GURL("https://www.foo1.org"),
                                 GURL("https://www.foo2.org")};

  // Act
  AntiTargetingExclusionRule frequency_cap(&resource, history);
  const bool should_exclude = frequency_cap.ShouldExclude(creative_ad);

  // Assert
  EXPECT_FALSE(should_exclude);
}

TEST_F(BatAdsAntiTargetingExclusionRuleTest,
       DoNotAllowIfCreativeSetAndSiteDoesMatch) {
  // Arrange
  CreativeAdInfo creative_ad;
  creative_ad.creative_set_id = kCreativeSetIdOnAntiTargetingList;

  resource::AntiTargeting resource;
  resource.Load();
  task_environment()->RunUntilIdle();

  BrowsingHistoryList history = {GURL("https://www.foo1.org"),
                                 GURL("https://www.brave.com")};

  // Act
  AntiTargetingExclusionRule frequency_cap(&resource, history);
  const bool should_exclude = frequency_cap.ShouldExclude(creative_ad);

  // Assert
  EXPECT_TRUE(should_exclude);
}

}  // namespace ads
