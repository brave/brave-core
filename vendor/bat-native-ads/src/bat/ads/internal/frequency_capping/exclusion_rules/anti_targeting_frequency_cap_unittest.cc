/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/frequency_capping/exclusion_rules/anti_targeting_frequency_cap.h"

#include <memory>

#include "base/strings/string_number_conversions.h"
#include "bat/ads/internal/resources/frequency_capping/anti_targeting_resource.h"
#include "bat/ads/internal/unittest_base.h"
#include "bat/ads/internal/unittest_util.h"

// npm run test -- brave_unit_tests --filter=BatAds*

namespace ads {

namespace {
const char kCreativeSetIdOnAntiTargetingList[] =
    "5bdeab83-048f-48a7-9602-a1092ded123c";
const char kCreativeSetIdNotOnAntiTargetingList[] =
    "d175cdfd-57bf-46c3-9b00-89eed71c6ae5";

}  // namespace

class BatAdsAntiTargetingFrequencyCapTest : public UnitTestBase {
 protected:
  BatAdsAntiTargetingFrequencyCapTest() = default;

  ~BatAdsAntiTargetingFrequencyCapTest() override = default;
};

TEST_F(BatAdsAntiTargetingFrequencyCapTest, AllowIfResourceDidNotLoad) {
  // Arrange
  CreativeAdInfo ad;
  ad.creative_set_id = kCreativeSetIdOnAntiTargetingList;

  resource::AntiTargeting resource;

  BrowsingHistoryList history = {{"https://www.foo1.org"},
                                 {"https://www.brave.com"},
                                 {"https://www.foo2.org"}};

  // Act
  AntiTargetingFrequencyCap frequency_cap(&resource, history);
  const bool should_exclude = frequency_cap.ShouldExclude(ad);

  // Assert
  EXPECT_FALSE(should_exclude);
}

TEST_F(BatAdsAntiTargetingFrequencyCapTest, AllowIfCreativeSetDoesNotMatch) {
  // Arrange
  CreativeAdInfo ad;
  ad.creative_set_id = kCreativeSetIdNotOnAntiTargetingList;

  resource::AntiTargeting resource;
  resource.Load();

  BrowsingHistoryList history = {{"https://www.foo1.org"},
                                 {"https://www.brave.com"},
                                 {"https://www.foo2.org"}};

  // Act
  AntiTargetingFrequencyCap frequency_cap(&resource, history);
  const bool should_exclude = frequency_cap.ShouldExclude(ad);

  // Assert
  EXPECT_FALSE(should_exclude);
}

TEST_F(BatAdsAntiTargetingFrequencyCapTest, AllowIfSiteDoesNotMatch) {
  // Arrange
  CreativeAdInfo ad;
  ad.creative_set_id = kCreativeSetIdOnAntiTargetingList;

  resource::AntiTargeting resource;
  resource.Load();

  BrowsingHistoryList history = {{"https://www.foo1.org"},
                                 {"https://www.foo2.org"}};

  // Act
  AntiTargetingFrequencyCap frequency_cap(&resource, history);
  const bool should_exclude = frequency_cap.ShouldExclude(ad);

  // Assert
  EXPECT_FALSE(should_exclude);
}

TEST_F(BatAdsAntiTargetingFrequencyCapTest,
       DoNotAllowIfCreativeSetAndSiteDoesMatch) {
  // Arrange
  CreativeAdInfo ad;
  ad.creative_set_id = kCreativeSetIdOnAntiTargetingList;

  resource::AntiTargeting resource;
  resource.Load();

  BrowsingHistoryList history = {{"https://www.foo1.org"},
                                 {"https://www.brave.com"}};

  // Act
  AntiTargetingFrequencyCap frequency_cap(&resource, history);
  const bool should_exclude = frequency_cap.ShouldExclude(ad);

  // Assert
  EXPECT_TRUE(should_exclude);
}

}  // namespace ads
