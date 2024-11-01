/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_USER_ENGAGEMENT_AD_EVENTS_NOTIFICATION_ADS_NOTIFICATION_AD_EVENT_HANDLER_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_USER_ENGAGEMENT_AD_EVENTS_NOTIFICATION_ADS_NOTIFICATION_AD_EVENT_HANDLER_H_

#include <string>

#include "base/check_op.h"
#include "base/functional/callback_forward.h"
#include "base/memory/raw_ptr.h"
#include "base/memory/weak_ptr.h"
#include "brave/components/brave_ads/core/internal/user_engagement/ad_events/notification_ads/notification_ad_event_handler_delegate.h"
#include "brave/components/brave_ads/core/mojom/brave_ads.mojom-forward.h"

namespace brave_ads {

using FireNotificationAdEventHandlerCallback = base::OnceCallback<void(
    bool success,
    const std::string& placement_id,
    const mojom::NotificationAdEventType mojom_ad_event_type)>;

struct NotificationAdInfo;

class NotificationAdEventHandler final
    : public NotificationAdEventHandlerDelegate {
 public:
  NotificationAdEventHandler();

  NotificationAdEventHandler(const NotificationAdEventHandler&) = delete;
  NotificationAdEventHandler& operator=(const NotificationAdEventHandler&) =
      delete;

  NotificationAdEventHandler(NotificationAdEventHandler&&) noexcept = delete;
  NotificationAdEventHandler& operator=(NotificationAdEventHandler&&) noexcept =
      delete;

  ~NotificationAdEventHandler() override;

  void SetDelegate(NotificationAdEventHandlerDelegate* delegate) {
    CHECK_EQ(delegate_, nullptr);
    delegate_ = delegate;
  }

  void FireEvent(const std::string& placement_id,
                 mojom::NotificationAdEventType mojom_ad_event_type,
                 FireNotificationAdEventHandlerCallback callback);

 private:
  void FireEventCallback(const NotificationAdInfo& ad,
                         mojom::NotificationAdEventType mojom_ad_event_type,
                         FireNotificationAdEventHandlerCallback callback,
                         bool success) const;

  void SuccessfullyFiredEvent(
      const NotificationAdInfo& ad,
      mojom::NotificationAdEventType mojom_ad_event_type,
      FireNotificationAdEventHandlerCallback callback) const;
  void FailedToFireEvent(const std::string& placement_id,
                         mojom::NotificationAdEventType mojom_ad_event_type,
                         FireNotificationAdEventHandlerCallback callback) const;

  void NotifyDidFireNotificationAdEvent(
      const NotificationAdInfo& ad,
      mojom::NotificationAdEventType mojom_ad_event_type) const;
  void NotifyFailedToFireNotificationAdEvent(
      const std::string& placement_id,
      mojom::NotificationAdEventType mojom_ad_event_type) const;

  raw_ptr<NotificationAdEventHandlerDelegate> delegate_ =
      nullptr;  // Not owned.

  base::WeakPtrFactory<NotificationAdEventHandler> weak_factory_{this};
};

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_USER_ENGAGEMENT_AD_EVENTS_NOTIFICATION_ADS_NOTIFICATION_AD_EVENT_HANDLER_H_
