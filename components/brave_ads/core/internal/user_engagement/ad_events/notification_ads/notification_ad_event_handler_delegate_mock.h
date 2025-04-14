/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_USER_ENGAGEMENT_AD_EVENTS_NOTIFICATION_ADS_NOTIFICATION_AD_EVENT_HANDLER_DELEGATE_MOCK_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_USER_ENGAGEMENT_AD_EVENTS_NOTIFICATION_ADS_NOTIFICATION_AD_EVENT_HANDLER_DELEGATE_MOCK_H_

#include <string>

#include "brave/components/brave_ads/core/internal/user_engagement/ad_events/notification_ads/notification_ad_event_handler_delegate.h"
#include "testing/gmock/include/gmock/gmock.h"

namespace brave_ads {

class NotificationAdEventHandlerDelegateMock
    : public NotificationAdEventHandlerDelegate {
 public:
  NotificationAdEventHandlerDelegateMock();

  NotificationAdEventHandlerDelegateMock(
      const NotificationAdEventHandlerDelegateMock&) = delete;
  NotificationAdEventHandlerDelegateMock& operator=(
      const NotificationAdEventHandlerDelegateMock&) = delete;

  ~NotificationAdEventHandlerDelegateMock() override;

  MOCK_METHOD(void,
              OnDidFireNotificationAdServedEvent,
              (const NotificationAdInfo&));
  MOCK_METHOD(void,
              OnDidFireNotificationAdViewedEvent,
              (const NotificationAdInfo&));
  MOCK_METHOD(void,
              OnDidFireNotificationAdClickedEvent,
              (const NotificationAdInfo&));
  MOCK_METHOD(void,
              OnDidFireNotificationAdDismissedEvent,
              (const NotificationAdInfo&));
  MOCK_METHOD(void,
              OnDidFireNotificationAdTimedOutEvent,
              (const NotificationAdInfo&));
  MOCK_METHOD(void,
              OnFailedToFireNotificationAdEvent,
              (const std::string&, mojom::NotificationAdEventType));
};

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_USER_ENGAGEMENT_AD_EVENTS_NOTIFICATION_ADS_NOTIFICATION_AD_EVENT_HANDLER_DELEGATE_MOCK_H_
