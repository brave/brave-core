/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/user_engagement/ad_events/notification_ads/notification_ad_event_handler.h"

#include <optional>
#include <utility>

#include "base/check.h"
#include "brave/components/brave_ads/core/internal/common/logging_util.h"
#include "brave/components/brave_ads/core/internal/creatives/notification_ads/notification_ad_manager.h"
#include "brave/components/brave_ads/core/internal/user_engagement/ad_events/notification_ads/notification_ad_event_factory.h"
#include "brave/components/brave_ads/core/public/ad_units/notification_ad/notification_ad_info.h"
#include "brave/components/brave_ads/core/public/ads_client/ads_client.h"

namespace brave_ads {

NotificationAdEventHandler::NotificationAdEventHandler() = default;

NotificationAdEventHandler::~NotificationAdEventHandler() {
  delegate_ = nullptr;
}

void NotificationAdEventHandler::FireEvent(
    const std::string& placement_id,
    const mojom::NotificationAdEventType mojom_ad_event_type,
    FireNotificationAdEventHandlerCallback callback) {
  CHECK(!placement_id.empty());

  const std::optional<NotificationAdInfo> ad =
      NotificationAdManager::GetInstance().MaybeGetForPlacementId(placement_id);
  if (!ad) {
    BLOG(1, "Failed to fire notification ad event due to missing placement id "
                << placement_id);

    return FailedToFireEvent(placement_id, mojom_ad_event_type,
                             std::move(callback));
  }

  const auto ad_event = NotificationAdEventFactory::Build(mojom_ad_event_type);
  ad_event->FireEvent(
      *ad, base::BindOnce(&NotificationAdEventHandler::FireEventCallback,
                          weak_factory_.GetWeakPtr(), *ad, mojom_ad_event_type,
                          std::move(callback)));
}

///////////////////////////////////////////////////////////////////////////////

void NotificationAdEventHandler::FireEventCallback(
    const NotificationAdInfo& ad,
    const mojom::NotificationAdEventType mojom_ad_event_type,
    FireNotificationAdEventHandlerCallback callback,
    const bool success) const {
  if (!success) {
    return FailedToFireEvent(ad.placement_id, mojom_ad_event_type,
                             std::move(callback));
  }

  SuccessfullyFiredEvent(ad, mojom_ad_event_type, std::move(callback));
}

void NotificationAdEventHandler::SuccessfullyFiredEvent(
    const NotificationAdInfo& ad,
    const mojom::NotificationAdEventType mojom_ad_event_type,
    FireNotificationAdEventHandlerCallback callback) const {
  NotifyDidFireNotificationAdEvent(ad, mojom_ad_event_type);

  std::move(callback).Run(/*success=*/true, ad.placement_id,
                          mojom_ad_event_type);
}

void NotificationAdEventHandler::FailedToFireEvent(
    const std::string& placement_id,
    const mojom::NotificationAdEventType mojom_ad_event_type,
    FireNotificationAdEventHandlerCallback callback) const {
  BLOG(1, "Failed to fire notification ad " << mojom_ad_event_type
                                            << " event for placement id "
                                            << placement_id);

  NotifyFailedToFireNotificationAdEvent(placement_id, mojom_ad_event_type);

  std::move(callback).Run(/*success=*/false, placement_id, mojom_ad_event_type);
}

void NotificationAdEventHandler::NotifyDidFireNotificationAdEvent(
    const NotificationAdInfo& ad,
    mojom::NotificationAdEventType mojom_ad_event_type) const {
  if (!delegate_) {
    return;
  }

  switch (mojom_ad_event_type) {
    case mojom::NotificationAdEventType::kServedImpression: {
      delegate_->OnDidFireNotificationAdServedEvent(ad);
      break;
    }

    case mojom::NotificationAdEventType::kViewedImpression: {
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

void NotificationAdEventHandler::NotifyFailedToFireNotificationAdEvent(
    const std::string& placement_id,
    const mojom::NotificationAdEventType mojom_ad_event_type) const {
  if (delegate_) {
    delegate_->OnFailedToFireNotificationAdEvent(placement_id,
                                                 mojom_ad_event_type);
  }
}

}  // namespace brave_ads
