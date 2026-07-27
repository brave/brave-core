/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/public/ad_units/notification_ad/notification_ad_util.h"

#include "brave/components/brave_ads/core/mojom/brave_ads.mojom.h"
#include "brave/components/brave_ads/core/public/ad_units/notification_ad/notification_ad_info.h"

namespace brave_ads {

mojom::NotificationAdInfoPtr ToMojom(
    base::optional_ref<const NotificationAdInfo> ad) {
  if (!ad || !ad->IsValid()) {
    return nullptr;
  }

  mojom::NotificationAdInfoPtr notification_ad =
      mojom::NotificationAdInfo::New();
  notification_ad->type = ad->type;
  notification_ad->placement_id = ad->placement_id;
  notification_ad->creative_instance_id = ad->creative_instance_id;
  notification_ad->creative_set_id = ad->creative_set_id;
  notification_ad->campaign_id = ad->campaign_id;
  notification_ad->advertiser_id = ad->advertiser_id;
  notification_ad->segment = ad->segment;
  notification_ad->title = ad->title;
  notification_ad->body = ad->body;
  notification_ad->target_url = ad->target_url;
  return notification_ad;
}

}  // namespace brave_ads
