/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/ads/ad_events/notification_ads/notification_ad_event_handler.h"

#include <utility>

#include "base/check.h"
#include "brave/components/brave_ads/core/internal/ads/ad_events/notification_ads/notification_ad_event_factory.h"
#include "brave/components/brave_ads/core/internal/common/logging_util.h"
#include "brave/components/brave_ads/core/internal/creatives/notification_ads/notification_ad_manager.h"
#include "brave/components/brave_ads/core/public/ads/notification_ad_info.h"
#include "third_party/abseil-cpp/absl/types/optional.h"

namespace brave_ads {

NotificationAdEventHandler::NotificationAdEventHandler() = default;

NotificationAdEventHandler::~NotificationAdEventHandler() {
  delegate_ = nullptr;
}

void NotificationAdEventHandler::FireEvent(
    const std::string& placement_id,
    const mojom::NotificationAdEventType event_type,
    FireNotificationAdEventHandlerCallback callback) {
  CHECK(!placement_id.empty());
  CHECK(mojom::IsKnownEnumValue(event_type));

  const absl::optional<NotificationAdInfo> ad =
      NotificationAdManager::GetInstance().MaybeGetForPlacementId(placement_id);
  if (!ad) {
    BLOG(1, "Failed to fire notification ad event due to missing placement id "
                << placement_id);
    return FailedToFireEvent(placement_id, event_type, std::move(callback));
  }

  const auto ad_event = NotificationAdEventFactory::Build(event_type);
  ad_event->FireEvent(
      *ad, base::BindOnce(&NotificationAdEventHandler::FireEventCallback,
                          weak_factory_.GetWeakPtr(), *ad, event_type,
                          std::move(callback)));
}

void NotificationAdEventHandler::FireEventCallback(
    const NotificationAdInfo& ad,
    const mojom::NotificationAdEventType event_type,
    FireNotificationAdEventHandlerCallback callback,
    const bool success) const {
  if (!success) {
    return FailedToFireEvent(ad.placement_id, event_type, std::move(callback));
  }

  SuccessfullyFiredEvent(ad, event_type, std::move(callback));
}

void NotificationAdEventHandler::SuccessfullyFiredEvent(
    const NotificationAdInfo& ad,
    const mojom::NotificationAdEventType event_type,
    FireNotificationAdEventHandlerCallback callback) const {
  CHECK(mojom::IsKnownEnumValue(event_type));

  if (delegate_) {
    switch (event_type) {
      case mojom::NotificationAdEventType::kServed: {
        delegate_->OnDidFireNotificationAdServedEvent(ad);
        break;
      }

      case mojom::NotificationAdEventType::kViewed: {
        delegate_->OnDidFireNotificationAdViewedEvent(ad);
        break;
      }

      case mojom::NotificationAdEventType::kClicked: {
        delegate_->OnDidFireNotificationAdClickedEvent(ad);
        break;
      }

      case mojom::NotificationAdEventType::kDismissed: {
        delegate_->OnDidFireNotificationAdDismissedEvent(ad);
        break;
      }

      case mojom::NotificationAdEventType::kTimedOut: {
        delegate_->OnDidFireNotificationAdTimedOutEvent(ad);
        break;
      }
    }
  }

  std::move(callback).Run(/*success*/ true, ad.placement_id, event_type);
}

void NotificationAdEventHandler::FailedToFireEvent(
    const std::string& placement_id,
    const mojom::NotificationAdEventType event_type,
    FireNotificationAdEventHandlerCallback callback) const {
  CHECK(mojom::IsKnownEnumValue(event_type));

  BLOG(1, "Failed to fire notification ad "
              << event_type << " event for placement id " << placement_id);

  if (delegate_) {
    delegate_->OnFailedToFireNotificationAdEvent(placement_id, event_type);
  }

  std::move(callback).Run(/*success*/ false, placement_id, event_type);
}

}  // namespace brave_ads
