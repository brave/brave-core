/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/conversions/conversion_queue_item_unittest_util.h"

#include "bat/ads/internal/database/tables/conversion_queue_database_table.h"
#include "bat/ads/internal/unittest_time_util.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace ads {

namespace {

constexpr char kCampaignId[] = "84197fc8-830a-4a8e-8339-7a70c2bfa104";
constexpr char kCreativeSetId[] = "c2ba3e7d-f688-4bc4-a053-cbe7ac1e6123";
constexpr char kCreativeInstanceId[] = "3519f52c-46a4-4c48-9c2b-c264c0067f04";
constexpr char kAdvertiserId[] = "5484a63f-eb99-4ba5-a3b0-8c25d3c0e4b2";

}  // namespace

void SaveConversionQueueItems(
    const ConversionQueueItemList& conversion_queue_items) {
  database::table::ConversionQueue database_table;
  database_table.Save(conversion_queue_items,
                      [](const bool success) { ASSERT_TRUE(success); });
}

ConversionQueueItemInfo BuildConversionQueueItem(
    const std::string& conversion_id,
    const std::string& advertiser_public_key) {
  ConversionQueueItemInfo conversion_queue_item;
  conversion_queue_item.campaign_id = kCampaignId;
  conversion_queue_item.creative_set_id = kCreativeSetId;
  conversion_queue_item.creative_instance_id = kCreativeInstanceId;
  conversion_queue_item.advertiser_id = kAdvertiserId;
  conversion_queue_item.conversion_id = conversion_id;
  conversion_queue_item.advertiser_public_key = advertiser_public_key;
  conversion_queue_item.ad_type = AdType::kAdNotification;
  conversion_queue_item.process_at = Now();

  return conversion_queue_item;
}

void BuildAndSaveConversionQueueItem(const std::string& conversion_id,
                                     const std::string& advertiser_public_key) {
  ConversionQueueItemList conversion_queue_items;

  const ConversionQueueItemInfo& conversion_queue_item =
      BuildConversionQueueItem(conversion_id, advertiser_public_key);

  conversion_queue_items.push_back(conversion_queue_item);

  SaveConversionQueueItems(conversion_queue_items);
}

}  // namespace ads
