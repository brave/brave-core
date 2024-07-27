/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/serving/eligible_ads/exclusion_rules/anti_targeting_exclusion_rule.h"

#include <memory>

#include "brave/components/brave_ads/core/internal/ad_units/ad_test_constants.h"
#include "brave/components/brave_ads/core/internal/common/resources/country_components_test_constants.h"
#include "brave/components/brave_ads/core/internal/common/test/test_base.h"
#include "brave/components/brave_ads/core/internal/creatives/creative_ad_info.h"
#include "brave/components/brave_ads/core/internal/targeting/behavioral/anti_targeting/resource/anti_targeting_resource.h"
#include "url/gurl.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

namespace {
constexpr char kAntiTargetedSite[] = "https://www.brave.com";
}  // namespace

class BraveAdsAntiTargetingExclusionRuleTest : public test::TestBase {
 protected:
  void SetUp() override {
    test::TestBase::SetUp();

    resource_ = std::make_unique<AntiTargetingResource>();
  }

  std::unique_ptr<AntiTargetingResource> resource_;
};

TEST_F(BraveAdsAntiTargetingExclusionRuleTest,
       ShouldIncludeIfResourceIsNotInitialized) {
  // Arrange
  const AntiTargetingExclusionRule exclusion_rule(
      *resource_, /*site_history=*/{GURL(kAntiTargetedSite)});

  CreativeAdInfo creative_ad;
  creative_ad.creative_set_id = test::kCreativeSetId;

  // Act & Assert
  EXPECT_TRUE(exclusion_rule.ShouldInclude(creative_ad).has_value());
}

TEST_F(BraveAdsAntiTargetingExclusionRuleTest,
       ShouldIncludeIfNotVisitedAntiTargetedSiteForCreativeSet) {
  // Arrange
  NotifyResourceComponentDidChange(test::kCountryComponentManifestVersion,
                                   test::kCountryComponentId);
  ASSERT_TRUE(resource_->IsLoaded());

  const AntiTargetingExclusionRule exclusion_rule(
      *resource_, /*site_history=*/{GURL(kAntiTargetedSite)});

  CreativeAdInfo creative_ad;
  creative_ad.creative_set_id = test::kMissingCreativeSetId;

  // Act & Assert
  EXPECT_TRUE(exclusion_rule.ShouldInclude(creative_ad).has_value());
}

TEST_F(BraveAdsAntiTargetingExclusionRuleTest,
       ShouldIncludeIfNotVisitedAntiTargetedSite) {
  // Arrange
  NotifyResourceComponentDidChange(test::kCountryComponentManifestVersion,
                                   test::kCountryComponentId);
  ASSERT_TRUE(resource_->IsLoaded());

  const AntiTargetingExclusionRule exclusion_rule(
      *resource_, /*site_history=*/{GURL("https://www.foo.com")});

  CreativeAdInfo creative_ad;
  creative_ad.creative_set_id = test::kCreativeSetId;

  // Act & Assert
  EXPECT_TRUE(exclusion_rule.ShouldInclude(creative_ad).has_value());
}

TEST_F(BraveAdsAntiTargetingExclusionRuleTest,
       ShouldExcludeIfVisitedAntiTargetedSite) {
  // Arrange
  NotifyResourceComponentDidChange(test::kCountryComponentManifestVersion,
                                   test::kCountryComponentId);
  ASSERT_TRUE(resource_->IsLoaded());

  const AntiTargetingExclusionRule exclusion_rule(
      *resource_, /*site_history=*/{GURL(kAntiTargetedSite)});

  CreativeAdInfo creative_ad;
  creative_ad.creative_set_id = test::kCreativeSetId;

  // Act & Assert
  EXPECT_FALSE(exclusion_rule.ShouldInclude(creative_ad).has_value());
}

}  // namespace brave_ads
