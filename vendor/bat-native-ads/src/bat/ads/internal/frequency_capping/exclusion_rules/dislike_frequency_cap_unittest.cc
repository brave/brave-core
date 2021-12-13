/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/frequency_capping/exclusion_rules/dislike_frequency_cap.h"

#include "bat/ads/ad_content_info.h"
#include "bat/ads/internal/client/client.h"
#include "bat/ads/internal/unittest_base.h"
#include "bat/ads/internal/unittest_util.h"

// npm run test -- brave_unit_tests --filter=BatAds*

namespace ads {

namespace {
const char kAdvertiserId[] = "1d3349f6-6713-4324-a135-b377237450a4";
}  // namespace

class BatAdsDislikeFrequencyCapTest : public UnitTestBase {
 protected:
  BatAdsDislikeFrequencyCapTest() = default;

  ~BatAdsDislikeFrequencyCapTest() override = default;
};

TEST_F(BatAdsDislikeFrequencyCapTest, AllowAd) {
  // Arrange
  CreativeAdInfo creative_ad;
  creative_ad.advertiser_id = kAdvertiserId;

  // Act
  DislikeFrequencyCap frequency_cap;
  const bool should_exclude = frequency_cap.ShouldExclude(creative_ad);

  // Assert
  EXPECT_FALSE(should_exclude);
}

TEST_F(BatAdsDislikeFrequencyCapTest, DoNotAllowAd) {
  // Arrange
  CreativeAdInfo creative_ad;
  creative_ad.advertiser_id = kAdvertiserId;

  AdContentInfo ad_content;
  ad_content.advertiser_id = kAdvertiserId;
  ad_content.like_action_type = AdContentLikeActionType::kNeutral;
  Client::Get()->ToggleAdThumbDown(ad_content);

  // Act
  DislikeFrequencyCap frequency_cap;
  const bool should_exclude = frequency_cap.ShouldExclude(creative_ad);

  // Assert
  EXPECT_TRUE(should_exclude);
}

}  // namespace ads
