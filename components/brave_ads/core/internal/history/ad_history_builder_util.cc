/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/history/ad_history_builder_util.h"

#include "base/time/time.h"
#include "brave/components/brave_ads/core/internal/deprecated/client/client_state_manager.h"
#include "brave/components/brave_ads/core/public/ad_units/ad_info.h"
#include "brave/components/brave_ads/core/public/history/ad_history_item_info.h"

namespace brave_ads {

AdHistoryItemInfo BuildAdHistoryItem(const AdInfo& ad,
                                     const ConfirmationType confirmation_type,
                                     const std::string& title,
                                     const std::string& description) {
  AdHistoryItemInfo ad_history_item;

  ad_history_item.created_at = base::Time::Now();

  ad_history_item.confirmation_type = confirmation_type;

  // Ad.
  ad_history_item.type = ad.type;
  ad_history_item.placement_id = ad.placement_id;
  ad_history_item.creative_instance_id = ad.creative_instance_id;
  ad_history_item.creative_set_id = ad.creative_set_id;
  ad_history_item.campaign_id = ad.campaign_id;
  ad_history_item.advertiser_id = ad.advertiser_id;
  ad_history_item.segment = ad.segment;

  // Brand.
  ad_history_item.brand = title;
  ad_history_item.brand_info = description;
  ad_history_item.brand_display_url = ad.target_url.host();
  ad_history_item.brand_url = ad.target_url;

  // User reactions.
  ad_history_item.ad_reaction_type =
      ClientStateManager::GetInstance().GetReactionTypeForAd(ad);
  ad_history_item.segment_reaction_type =
      ClientStateManager::GetInstance().GetReactionTypeForSegment(ad.segment);
  ad_history_item.is_saved = false;
  ad_history_item.is_marked_as_inappropriate = false;

  return ad_history_item;
}

}  // namespace brave_ads
