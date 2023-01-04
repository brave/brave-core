/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/history/ad_content_util.h"

#include "bat/ads/ad_content_info.h"
#include "bat/ads/ad_info.h"
#include "bat/ads/confirmation_type.h"
#include "bat/ads/internal/deprecated/client/client_state_manager.h"

namespace ads {

AdContentInfo BuildAdContent(const AdInfo& ad,
                             const ConfirmationType& confirmation_type,
                             const std::string& title,
                             const std::string& description) {
  AdContentInfo ad_content;

  ad_content.type = ad.type;
  ad_content.placement_id = ad.placement_id;
  ad_content.creative_instance_id = ad.creative_instance_id;
  ad_content.creative_set_id = ad.creative_set_id;
  ad_content.campaign_id = ad.campaign_id;
  ad_content.advertiser_id = ad.advertiser_id;
  ad_content.brand = title;
  ad_content.brand_info = description;
  ad_content.brand_display_url = ad.target_url.host();
  ad_content.brand_url = ad.target_url;
  ad_content.like_action_type =
      ClientStateManager::GetInstance()
          ->GetAdContentLikeActionTypeForAdvertiser(ad.advertiser_id);
  ad_content.confirmation_type = confirmation_type;

  return ad_content;
}

}  // namespace ads
