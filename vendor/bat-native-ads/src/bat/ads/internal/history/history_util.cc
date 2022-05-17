/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/history/history_util.h"

#include "base/time/time.h"
#include "bat/ads/ad_info.h"
#include "bat/ads/confirmation_type.h"
#include "bat/ads/history_item_info.h"
#include "bat/ads/internal/deprecated/client/client.h"

namespace ads {

HistoryItemInfo BuildHistoryItem(const AdInfo& ad,
                                 const ConfirmationType& confirmation_type,
                                 const std::string& title,
                                 const std::string& description) {
  HistoryItemInfo history_item;

  history_item.time = base::Time::Now();

  history_item.ad_content.type = ad.type;
  history_item.ad_content.placement_id = ad.placement_id;
  history_item.ad_content.creative_instance_id = ad.creative_instance_id;
  history_item.ad_content.creative_set_id = ad.creative_set_id;
  history_item.ad_content.campaign_id = ad.campaign_id;
  history_item.ad_content.advertiser_id = ad.advertiser_id;
  history_item.ad_content.brand = title;
  history_item.ad_content.brand_info = description;
  history_item.ad_content.brand_display_url = ad.target_url.host();
  history_item.ad_content.brand_url = ad.target_url;
  history_item.ad_content.like_action_type =
      Client::Get()->GetAdContentLikeActionTypeForAdvertiser(ad.advertiser_id);
  history_item.ad_content.confirmation_type = confirmation_type;
  history_item.category_content.opt_action_type =
      Client::Get()->GetCategoryContentOptActionTypeForSegment(ad.segment);
  history_item.category_content.category = ad.segment;

  return history_item;
}

}  // namespace ads
