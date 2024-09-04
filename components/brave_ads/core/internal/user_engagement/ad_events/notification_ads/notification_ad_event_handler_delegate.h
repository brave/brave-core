/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_USER_ENGAGEMENT_AD_EVENTS_NOTIFICATION_ADS_NOTIFICATION_AD_EVENT_HANDLER_DELEGATE_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_USER_ENGAGEMENT_AD_EVENTS_NOTIFICATION_ADS_NOTIFICATION_AD_EVENT_HANDLER_DELEGATE_H_

#include <string>

#include "brave/components/brave_ads/core/mojom/brave_ads.mojom-forward.h"

namespace brave_ads {

struct NotificationAdInfo;

class NotificationAdEventHandlerDelegate {
 public:
  // Invoked when the notification `ad` is served.
  virtual void OnDidFireNotificationAdServedEvent(
      const NotificationAdInfo& ad) {}

  // Invoked when the notification `ad` is viewed.
  virtual void OnDidFireNotificationAdViewedEvent(
      const NotificationAdInfo& ad) {}

  // Invoked when the notification `ad` is clicked.
  virtual void OnDidFireNotificationAdClickedEvent(
      const NotificationAdInfo& ad) {}

  // Invoked when the notification `ad` is dismissed.
  virtual void OnDidFireNotificationAdDismissedEvent(
      const NotificationAdInfo& ad) {}

  // Invoked when the notification `ad` times out.
  virtual void OnDidFireNotificationAdTimedOutEvent(
      const NotificationAdInfo& ad) {}

  // Invoked when the notification ad event fails for `placement_id` and
  // `mojom_ad_event_type`.
  virtual void OnFailedToFireNotificationAdEvent(
      const std::string& placement_id,
      const mojom::NotificationAdEventType mojom_ad_event_type) {}

 protected:
  virtual ~NotificationAdEventHandlerDelegate() = default;
};

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_USER_ENGAGEMENT_AD_EVENTS_NOTIFICATION_ADS_NOTIFICATION_AD_EVENT_HANDLER_DELEGATE_H_
