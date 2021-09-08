/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/ads_history/ads_history_util.h"

#include <cstdint>

#include "base/time/time.h"
#include "bat/ads/ad_info.h"
#include "bat/ads/ads_history_info.h"
#include "bat/ads/confirmation_type.h"
#include "bat/ads/internal/unittest_base.h"
#include "bat/ads/internal/unittest_util.h"
#include "bat/ads/internal/url_util.h"

// npm run test -- brave_unit_tests --filter=BatAds*

namespace ads {

class BatAdsAdsHistoryUtilTest : public UnitTestBase {
 protected:
  BatAdsAdsHistoryUtilTest() = default;

  ~BatAdsAdsHistoryUtilTest() override = default;
};

TEST_F(BatAdsAdsHistoryUtilTest, BuildAd) {
  // Arrange
  AdInfo ad;
  ad.type = AdType::kAdNotification;
  ad.uuid = "cdc6855f-d76d-4eb4-9b91-64247fed2f66";
  ad.creative_instance_id = "c7a368fd-572d-4af8-be4c-3966475a29b3";
  ad.creative_set_id = "121e5e50-4397-4128-ae38-47525bc1d421";
  ad.campaign_id = "e0fc8a2d-db96-44fb-8522-d299cb98559e";
  ad.advertiser_id = "49e008eb-5e37-4828-975f-e0de3a017b02";
  ad.segment = "technology & computing-software";
  ad.target_url = "https://brave.com";

  // Act
  const AdHistoryInfo ad_history =
      BuildAdHistory(ad, ConfirmationType::kViewed, "title", "body");

  // Assert
  AdHistoryInfo expected_ad_history;

  expected_ad_history.timestamp_in_seconds =
      static_cast<uint64_t>(base::Time::Now().ToDoubleT());

  expected_ad_history.ad_content.type = ad.type;
  expected_ad_history.ad_content.uuid = ad.uuid;
  expected_ad_history.ad_content.creative_instance_id = ad.creative_instance_id;
  expected_ad_history.ad_content.creative_set_id = ad.creative_set_id;
  expected_ad_history.ad_content.campaign_id = ad.campaign_id;
  expected_ad_history.ad_content.brand = "title";
  expected_ad_history.ad_content.brand_info = "body";
  expected_ad_history.ad_content.brand_display_url =
      GetHostFromUrl(ad.target_url);
  expected_ad_history.ad_content.brand_url = ad.target_url;
  expected_ad_history.ad_content.ad_action = ConfirmationType::kViewed;
  expected_ad_history.ad_content.like_action =
      AdContentInfo::LikeAction::kNeutral;

  expected_ad_history.category_content.category = ad.segment;
  expected_ad_history.category_content.opt_action =
      CategoryContentInfo::OptAction::kNone;

  EXPECT_EQ(expected_ad_history, ad_history);
}

}  // namespace ads
