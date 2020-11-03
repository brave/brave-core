/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/ad_delivery/ad_notifications/ad_notification_delivery.h"

#include <vector>

#include "bat/ads/ad_notification_info.h"
#include "bat/ads/internal/ads/ad_notifications/ad_notifications.h"
#include "bat/ads/internal/ads_impl.h"
#include "bat/ads/internal/client/client.h"
#include "bat/ads/internal/logging.h"
#include "bat/ads/internal/p2a/p2a.h"
#include "bat/ads/internal/p2a/p2a_util.h"

namespace ads {
namespace ad_notifications {

AdDelivery::AdDelivery(
    AdsImpl* ads)
    : ads_(ads) {
  DCHECK(ads_);
}

AdDelivery::~AdDelivery() = default;

bool AdDelivery::MaybeDeliverAd(
    const AdNotificationInfo& ad) {
  if (!ad.IsValid()) {
    return false;
  }

  ads_->get_client()->UpdateSeenAdNotification(ad.creative_instance_id);

  RecordAdImpressionForCategory(ad.category);

  ShowNotification(ad);

  return true;
}

///////////////////////////////////////////////////////////////////////////////

void AdDelivery::ShowNotification(
    const AdNotificationInfo& ad) {
  ads_->get_ad_notifications()->PushBack(ad);
}

void AdDelivery::RecordAdImpressionForCategory(
    const std::string& category) {
  const std::vector<std::string> question_list =
      CreateAdImpressionQuestionList(category);

  P2A p2a(ads_);
  p2a.RecordEvent("ad_impression", question_list);
}

}  // namespace ad_notifications
}  // namespace ads
