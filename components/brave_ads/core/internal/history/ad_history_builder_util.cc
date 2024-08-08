/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/history/ad_history_builder_util.h"

#include "base/time/time.h"
#include "brave/components/brave_ads/core/public/ad_units/ad_info.h"
#include "brave/components/brave_ads/core/public/history/ad_history_item_info.h"

namespace brave_ads {

AdHistoryItemInfo BuildAdHistoryItem(const AdInfo& ad,
                                     const ConfirmationType confirmation_type,
                                     const std::string& title,
                                     const std::string& description) {
  CHECK(ad.IsValid());

  AdHistoryItemInfo ad_history_item;

  ad_history_item.created_at = base::Time::Now();
  ad_history_item.type = ad.type;
  ad_history_item.confirmation_type = confirmation_type;
  ad_history_item.placement_id = ad.placement_id;
  ad_history_item.creative_instance_id = ad.creative_instance_id;
  ad_history_item.creative_set_id = ad.creative_set_id;
  ad_history_item.campaign_id = ad.campaign_id;
  ad_history_item.advertiser_id = ad.advertiser_id;
  ad_history_item.segment = ad.segment;
  ad_history_item.title = title;
  ad_history_item.description = description;
  ad_history_item.target_url = ad.target_url;

  return ad_history_item;
}

}  // namespace brave_ads
