/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/conversions/conversion_queue_item_unittest_util.h"

#include "base/check.h"
#include "base/functional/bind.h"
#include "base/uuid.h"
#include "brave/components/brave_ads/core/ad_type.h"
#include "brave/components/brave_ads/core/internal/ads/ad_unittest_constants.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_time_util.h"
#include "brave/components/brave_ads/core/internal/conversions/conversion_queue_database_table.h"

namespace brave_ads {

void SaveConversionQueueItems(
    const ConversionQueueItemList& conversion_queue_items) {
  database::table::ConversionQueue database_table;
  database_table.Save(
      conversion_queue_items,
      base::BindOnce([](const bool success) { CHECK(success); }));
}

ConversionQueueItemInfo BuildConversionQueueItem(
    const AdType& ad_type,
    const std::string& conversion_id,
    const std::string& advertiser_public_key,
    const bool should_use_random_uuids) {
  ConversionQueueItemInfo conversion_queue_item;

  conversion_queue_item.ad_type = ad_type;

  conversion_queue_item.creative_instance_id =
      should_use_random_uuids
          ? base::Uuid::GenerateRandomV4().AsLowercaseString()
          : kCreativeInstanceId;

  conversion_queue_item.creative_set_id =
      should_use_random_uuids
          ? base::Uuid::GenerateRandomV4().AsLowercaseString()
          : kCreativeSetId;

  conversion_queue_item.campaign_id =
      should_use_random_uuids
          ? base::Uuid::GenerateRandomV4().AsLowercaseString()
          : kCampaignId;

  conversion_queue_item.advertiser_id =
      should_use_random_uuids
          ? base::Uuid::GenerateRandomV4().AsLowercaseString()
          : kAdvertiserId;

  conversion_queue_item.segment = kSegment;

  conversion_queue_item.conversion_id = conversion_id;

  conversion_queue_item.advertiser_public_key = advertiser_public_key;

  conversion_queue_item.process_at = Now();

  return conversion_queue_item;
}

ConversionQueueItemList BuildAndSaveConversionQueueItems(
    const AdType& ad_type,
    const std::string& conversion_id,
    const std::string& advertiser_public_key,
    const bool should_use_random_uuids,
    const size_t count) {
  ConversionQueueItemList conversion_queue_items;

  for (size_t i = 0; i < count; i++) {
    const ConversionQueueItemInfo conversion_queue_item =
        BuildConversionQueueItem(ad_type, conversion_id, advertiser_public_key,
                                 should_use_random_uuids);

    conversion_queue_items.push_back(conversion_queue_item);
  }

  SaveConversionQueueItems(conversion_queue_items);

  return conversion_queue_items;
}

}  // namespace brave_ads
