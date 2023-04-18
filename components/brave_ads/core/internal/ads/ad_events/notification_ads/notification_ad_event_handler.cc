/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/ads/ad_events/notification_ads/notification_ad_event_handler.h"

#include "base/check.h"
#include "brave/components/brave_ads/core/internal/ads/ad_events/notification_ads/notification_ad_event_factory.h"
#include "brave/components/brave_ads/core/internal/common/logging_util.h"
#include "brave/components/brave_ads/core/internal/creatives/notification_ads/notification_ad_manager.h"
#include "brave/components/brave_ads/core/notification_ad_info.h"
#include "third_party/abseil-cpp/absl/types/optional.h"

namespace brave_ads::notification_ads {

EventHandler::EventHandler() = default;

EventHandler::~EventHandler() {
  delegate_ = nullptr;
}

void EventHandler::FireEvent(const std::string& placement_id,
                             const mojom::NotificationAdEventType event_type) {
  DCHECK(!placement_id.empty());
  DCHECK(mojom::IsKnownEnumValue(event_type));

  const absl::optional<NotificationAdInfo> ad =
      NotificationAdManager::GetInstance()->MaybeGetForPlacementId(
          placement_id);
  if (!ad) {
    BLOG(1, "Failed to fire notification ad event due to missing placement id "
                << placement_id);
    return FailedToFireEvent(placement_id, event_type);
  }

  const auto ad_event = AdEventFactory::Build(event_type);
  ad_event->FireEvent(*ad);

  SuccessfullyFiredEvent(*ad, event_type);
}

///////////////////////////////////////////////////////////////////////////////

void EventHandler::SuccessfullyFiredEvent(
    const NotificationAdInfo& ad,
    const mojom::NotificationAdEventType event_type) const {
  DCHECK(mojom::IsKnownEnumValue(event_type));

  if (!delegate_) {
    return;
  }

  switch (event_type) {
    case mojom::NotificationAdEventType::kServed: {
      delegate_->OnNotificationAdServed(ad);
      break;
    }

    case mojom::NotificationAdEventType::kViewed: {
      delegate_->OnNotificationAdViewed(ad);
      break;
    }

    case mojom::NotificationAdEventType::kClicked: {
      delegate_->OnNotificationAdClicked(ad);
      break;
    }

    case mojom::NotificationAdEventType::kDismissed: {
      delegate_->OnNotificationAdDismissed(ad);
      break;
    }

    case mojom::NotificationAdEventType::kTimedOut: {
      delegate_->OnNotificationAdTimedOut(ad);
      break;
    }
  }
}

void EventHandler::FailedToFireEvent(
    const std::string& placement_id,
    const mojom::NotificationAdEventType event_type) const {
  DCHECK(mojom::IsKnownEnumValue(event_type));

  BLOG(1, "Failed to fire notification ad "
              << event_type << " event for placement id " << placement_id);

  if (delegate_) {
    delegate_->OnNotificationAdEventFailed(placement_id, event_type);
  }
}

}  // namespace brave_ads::notification_ads
