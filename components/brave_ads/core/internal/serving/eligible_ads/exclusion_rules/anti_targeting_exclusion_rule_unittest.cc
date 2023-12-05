/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/serving/eligible_ads/exclusion_rules/anti_targeting_exclusion_rule.h"

#include <memory>

#include "brave/components/brave_ads/core/internal/ad_units/ad_unittest_constants.h"
#include "brave/components/brave_ads/core/internal/common/resources/country_components_unittest_constants.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_base.h"
#include "brave/components/brave_ads/core/internal/creatives/creative_ad_info.h"
#include "brave/components/brave_ads/core/internal/targeting/behavioral/anti_targeting/resource/anti_targeting_resource.h"
#include "url/gurl.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

namespace {
constexpr char kAntiTargetedSite[] = "https://www.brave.com";
}  // namespace

class BraveAdsAntiTargetingExclusionRuleTest : public UnitTestBase {
 protected:
  void SetUp() override {
    UnitTestBase::SetUp();

    resource_ = std::make_unique<AntiTargetingResource>();
  }

  bool LoadResource() {
    NotifyDidUpdateResourceComponent(kCountryComponentManifestVersion,
                                     kCountryComponentId);
    task_environment_.RunUntilIdle();
    return resource_->IsInitialized();
  }

  std::unique_ptr<AntiTargetingResource> resource_;
};

TEST_F(BraveAdsAntiTargetingExclusionRuleTest,
       ShouldIncludeIfResourceIsNotInitialized) {
  // Arrange
  const AntiTargetingExclusionRule exclusion_rule(
      *resource_, /*browsing_history=*/{GURL(kAntiTargetedSite)});

  CreativeAdInfo creative_ad;
  creative_ad.creative_set_id = kCreativeSetId;

  // Act & Assert
  EXPECT_TRUE(exclusion_rule.ShouldInclude(creative_ad).has_value());
}

TEST_F(BraveAdsAntiTargetingExclusionRuleTest,
       ShouldIncludeIfNotVisitedAntiTargetedSiteForCreativeSet) {
  // Arrange
  ASSERT_TRUE(LoadResource());

  const AntiTargetingExclusionRule exclusion_rule(
      *resource_, /*browsing_history=*/{GURL(kAntiTargetedSite)});

  CreativeAdInfo creative_ad;
  creative_ad.creative_set_id = kMissingCreativeSetId;

  // Act & Assert
  EXPECT_TRUE(exclusion_rule.ShouldInclude(creative_ad).has_value());
}

TEST_F(BraveAdsAntiTargetingExclusionRuleTest,
       ShouldIncludeIfNotVisitedAntiTargetedSite) {
  // Arrange
  ASSERT_TRUE(LoadResource());

  const AntiTargetingExclusionRule exclusion_rule(
      *resource_, /*browsing_history=*/{GURL("https://www.foo.com")});

  CreativeAdInfo creative_ad;
  creative_ad.creative_set_id = kCreativeSetId;

  // Act & Assert
  EXPECT_TRUE(exclusion_rule.ShouldInclude(creative_ad).has_value());
}

TEST_F(BraveAdsAntiTargetingExclusionRuleTest,
       ShouldExcludeIfVisitedAntiTargetedSite) {
  // Arrange
  ASSERT_TRUE(LoadResource());

  const AntiTargetingExclusionRule exclusion_rule(
      *resource_, /*browsing_history=*/{GURL(kAntiTargetedSite)});

  CreativeAdInfo creative_ad;
  creative_ad.creative_set_id = kCreativeSetId;

  // Act & Assert
  EXPECT_FALSE(exclusion_rule.ShouldInclude(creative_ad).has_value());
}

}  // namespace brave_ads
