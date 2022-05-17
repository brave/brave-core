/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/history/history_util.h"

#include "bat/ads/ad_info.h"
#include "bat/ads/confirmation_type.h"
#include "bat/ads/history_info.h"
#include "bat/ads/history_item_info.h"
#include "bat/ads/internal/base/unittest_base.h"
#include "bat/ads/internal/base/unittest_time_util.h"
#include "bat/ads/internal/base/unittest_util.h"

// npm run test -- brave_unit_tests --filter=BatAds*

namespace ads {

class BatAdsHistoryUtilTest : public UnitTestBase {
 protected:
  BatAdsHistoryUtilTest() = default;

  ~BatAdsHistoryUtilTest() override = default;
};

TEST_F(BatAdsHistoryUtilTest, BuildAd) {
  // Arrange
  AdInfo ad;
  ad.type = AdType::kAdNotification;
  ad.placement_id = "56b604b7-5eeb-4b7f-84cc-bf965556a550";
  ad.creative_instance_id = "c7a368fd-572d-4af8-be4c-3966475a29b3";
  ad.creative_set_id = "121e5e50-4397-4128-ae38-47525bc1d421";
  ad.campaign_id = "e0fc8a2d-db96-44fb-8522-d299cb98559e";
  ad.advertiser_id = "49e008eb-5e37-4828-975f-e0de3a017b02";
  ad.segment = "technology & computing-software";
  ad.target_url = GURL("https://brave.com");

  // Act
  const HistoryItemInfo& history_item =
      BuildHistoryItem(ad, ConfirmationType::kViewed, "title", "description");

  // Assert
  HistoryItemInfo expected_history_item;

  expected_history_item.time = Now();
  expected_history_item.ad_content.type = ad.type;
  expected_history_item.ad_content.placement_id = ad.placement_id;
  expected_history_item.ad_content.creative_instance_id =
      ad.creative_instance_id;
  expected_history_item.ad_content.creative_set_id = ad.creative_set_id;
  expected_history_item.ad_content.campaign_id = ad.campaign_id;
  expected_history_item.ad_content.advertiser_id = ad.advertiser_id;
  expected_history_item.ad_content.brand = "title";
  expected_history_item.ad_content.brand_info = "description";
  expected_history_item.ad_content.brand_display_url = ad.target_url.host();
  expected_history_item.ad_content.brand_url = ad.target_url;
  expected_history_item.ad_content.confirmation_type =
      ConfirmationType::kViewed;
  expected_history_item.ad_content.like_action_type =
      AdContentLikeActionType::kNeutral;

  expected_history_item.category_content.category = ad.segment;
  expected_history_item.category_content.opt_action_type =
      CategoryContentOptActionType::kNone;

  EXPECT_EQ(expected_history_item, history_item);
}

}  // namespace ads
