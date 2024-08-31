/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/serving/eligible_ads/exclusion_rules/anti_targeting_exclusion_rule_util.h"

#include "brave/components/brave_ads/core/internal/history/site_history_test_util.h"
#include "brave/components/brave_ads/core/public/history/site_history.h"
#include "testing/gtest/include/gtest/gtest.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

TEST(BraveAdsAntiTargetingExclusionRuleUtilTest, HasVisitedAntiTargetedSites) {
  // Arrange
  const SiteHistoryList site_history = test::BuildSiteHistory();

  const AntiTargetingSiteList anti_targeting_sites = {
      GURL("https://www.foo.com"), GURL("https://www.bar.com")};

  // Act & Assert
  EXPECT_TRUE(HasVisitedAntiTargetedSites(site_history, anti_targeting_sites));
}

TEST(BraveAdsAntiTargetingExclusionRuleUtilTest,
     HasVisitedCaseInsensitiveAntiTargetedSites) {
  // Arrange
  const SiteHistoryList site_history = test::BuildSiteHistory();

  const AntiTargetingSiteList anti_targeting_sites = {
      GURL("HTTPS://WWW.FOO.COM"), GURL("HTTPS://WWW.BAR.COM")};

  // Act & Assert
  EXPECT_TRUE(HasVisitedAntiTargetedSites(site_history, anti_targeting_sites));
}

TEST(BraveAdsAntiTargetingExclusionRuleUtilTest,
     HasNotVisitedAntiTargetedSites) {
  // Arrange
  const SiteHistoryList site_history = test::BuildSiteHistory();

  const AntiTargetingSiteList anti_targeting_sites = {
      GURL("https://www.brave.com"),
      GURL("https://www.basicattentiontoken.org")};

  // Act & Assert
  EXPECT_FALSE(HasVisitedAntiTargetedSites(site_history, anti_targeting_sites));
}

TEST(BraveAdsAntiTargetingExclusionRuleUtilTest,
     HasNotVisitedAntiTargetedInvalidSites) {
  // Arrange
  const SiteHistoryList site_history = test::BuildSiteHistory();

  const AntiTargetingSiteList anti_targeting_sites = {GURL("INVALID")};

  // Act & Assert
  EXPECT_FALSE(HasVisitedAntiTargetedSites(site_history, anti_targeting_sites));
}

TEST(BraveAdsAntiTargetingExclusionRuleUtilTest, HasNotVisitedIfNoSiteHistory) {
  // Arrange
  const AntiTargetingSiteList anti_targeting_sites = {GURL("INVALID")};

  // Act & Assert
  EXPECT_FALSE(
      HasVisitedAntiTargetedSites(/*site_history=*/{}, anti_targeting_sites));
}

TEST(BraveAdsAntiTargetingExclusionRuleUtilTest,
     HasNotVisitedIfNoAntiTargetedSites) {
  // Arrange
  const SiteHistoryList site_history = test::BuildSiteHistory();

  // Act & Assert
  EXPECT_FALSE(HasVisitedAntiTargetedSites(site_history, /*sites=*/{}));
}

TEST(BraveAdsAntiTargetingExclusionRuleUtilTest,
     HasNotVisitedIfNoSiteHistoryAndAntiTargetedSites) {
  // Act & Assert
  EXPECT_FALSE(HasVisitedAntiTargetedSites(/*site_history=*/{}, /*sites=*/{}));
}

}  // namespace brave_ads
