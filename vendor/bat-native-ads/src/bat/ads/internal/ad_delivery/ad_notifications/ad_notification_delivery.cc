/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/ad_delivery/ad_notifications/ad_notification_delivery.h"

#include <vector>

#include "bat/ads/ad_notification_info.h"
#include "bat/ads/internal/ads/ad_notifications/ad_notifications.h"
#include "bat/ads/internal/ads_client_helper.h"
#include "bat/ads/internal/client/client.h"
#include "bat/ads/internal/logging.h"
#include "bat/ads/internal/p2a/p2a.h"
#include "bat/ads/internal/p2a/p2a_util.h"

namespace ads {
namespace ad_notifications {

AdDelivery::AdDelivery() = default;

AdDelivery::~AdDelivery() = default;

bool AdDelivery::MaybeDeliverAd(
    const AdNotificationInfo& ad) {
  if (!ad.IsValid()) {
    return false;
  }

  Client::Get()->UpdateSeenAdNotification(ad.creative_instance_id);

  RecordAdImpressionForSegment(ad.segment);

  ShowNotification(ad);

  return true;
}

///////////////////////////////////////////////////////////////////////////////

void AdDelivery::ShowNotification(
    const AdNotificationInfo& ad) {
  AdNotifications::Get()->PushBack(ad);

  AdsClientHelper::Get()->ShowNotification(ad);
}

void AdDelivery::RecordAdImpressionForSegment(
    const std::string& segment) {
  const std::vector<std::string> question_list =
      p2a::CreateAdImpressionQuestionList(segment);

  p2a::RecordEvent("ad_impression", question_list);
}

}  // namespace ad_notifications
}  // namespace ads
