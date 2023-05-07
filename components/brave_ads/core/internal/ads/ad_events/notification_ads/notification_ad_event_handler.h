/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ADS_AD_EVENTS_NOTIFICATION_ADS_NOTIFICATION_AD_EVENT_HANDLER_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ADS_AD_EVENTS_NOTIFICATION_ADS_NOTIFICATION_AD_EVENT_HANDLER_H_

#include <string>

#include "base/check_op.h"
#include "base/memory/raw_ptr.h"
#include "brave/components/brave_ads/common/interfaces/brave_ads.mojom-shared.h"
#include "brave/components/brave_ads/core/internal/ads/ad_events/notification_ads/notification_ad_event_handler_delegate.h"

namespace brave_ads {

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
                 mojom::NotificationAdEventType event_type);

 private:
  void SuccessfullyFiredEvent(const NotificationAdInfo& ad,
                              mojom::NotificationAdEventType event_type) const;
  void FailedToFireEvent(const std::string& placement_id,
                         mojom::NotificationAdEventType event_type) const;

  raw_ptr<NotificationAdEventHandlerDelegate> delegate_ = nullptr;
};

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ADS_AD_EVENTS_NOTIFICATION_ADS_NOTIFICATION_AD_EVENT_HANDLER_H_
