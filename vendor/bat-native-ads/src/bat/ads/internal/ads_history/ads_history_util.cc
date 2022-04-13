/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/ads_history/ads_history_util.h"

#include "base/time/time.h"
#include "bat/ads/ad_history_info.h"
#include "bat/ads/ad_info.h"
#include "bat/ads/confirmation_type.h"
#include "bat/ads/internal/client/client.h"
#include "bat/ads/internal/url_util.h"

namespace ads {

AdHistoryInfo BuildAdHistory(const AdInfo& ad,
                             const ConfirmationType& confirmation_type,
                             const std::string& title,
                             const std::string& description) {
  AdHistoryInfo ad_history;

  ad_history.timestamp = base::Time::Now().ToDoubleT();

  ad_history.ad_content.type = ad.type;
  ad_history.ad_content.uuid = ad.placement_id;
  ad_history.ad_content.creative_instance_id = ad.creative_instance_id;
  ad_history.ad_content.creative_set_id = ad.creative_set_id;
  ad_history.ad_content.campaign_id = ad.campaign_id;
  ad_history.ad_content.advertiser_id = ad.advertiser_id;
  ad_history.ad_content.brand = title;
  ad_history.ad_content.brand_info = description;
  ad_history.ad_content.brand_display_url = GetHostFromUrl(ad.target_url);
  ad_history.ad_content.brand_url = ad.target_url;
  ad_history.ad_content.like_action_type =
      Client::Get()->GetAdContentLikeActionTypeForAdvertiser(ad.advertiser_id);
  ad_history.ad_content.confirmation_type = confirmation_type;
  ad_history.category_content.opt_action_type =
      Client::Get()->GetCategoryContentOptActionTypeForSegment(ad.segment);
  ad_history.category_content.category = ad.segment;

  return ad_history;
}

}  // namespace ads
