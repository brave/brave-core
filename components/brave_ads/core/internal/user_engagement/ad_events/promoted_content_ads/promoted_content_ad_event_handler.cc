/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/user_engagement/ad_events/promoted_content_ads/promoted_content_ad_event_handler.h"

#include <utility>

#include "base/functional/bind.h"
#include "brave/components/brave_ads/core/internal/ad_units/promoted_content_ad/promoted_content_ad_info.h"
#include "brave/components/brave_ads/core/internal/common/logging_util.h"
#include "brave/components/brave_ads/core/internal/creatives/promoted_content_ads/creative_promoted_content_ad_info.h"
#include "brave/components/brave_ads/core/internal/creatives/promoted_content_ads/promoted_content_ad_builder.h"
#include "brave/components/brave_ads/core/internal/serving/permission_rules/promoted_content_ads/promoted_content_ad_permission_rules.h"
#include "brave/components/brave_ads/core/internal/user_engagement/ad_events/ad_event_handler_util.h"
#include "brave/components/brave_ads/core/internal/user_engagement/ad_events/ad_events_database_table.h"
#include "brave/components/brave_ads/core/internal/user_engagement/ad_events/promoted_content_ads/promoted_content_ad_event_factory.h"

namespace brave_ads {

PromotedContentAdEventHandler::PromotedContentAdEventHandler() = default;

PromotedContentAdEventHandler::~PromotedContentAdEventHandler() {
  delegate_ = nullptr;
}

void PromotedContentAdEventHandler::FireEvent(
    const std::string& placement_id,
    const std::string& creative_instance_id,
    const mojom::PromotedContentAdEventType mojom_ad_event_type,
    FirePromotedContentAdEventHandlerCallback callback) {
  if (placement_id.empty()) {
    BLOG(1,
         "Failed to fire promoted content ad event due to an invalid placement "
         "id");
    return FailedToFireEvent(placement_id, creative_instance_id,
                             mojom_ad_event_type, std::move(callback));
  }

  if (creative_instance_id.empty()) {
    BLOG(1,
         "Failed to fire promoted content ad event due to an invalid creative "
         "instance id");
    return FailedToFireEvent(placement_id, creative_instance_id,
                             mojom_ad_event_type, std::move(callback));
  }

  GetAdEvents(placement_id, creative_instance_id, mojom_ad_event_type,
              std::move(callback));
}

///////////////////////////////////////////////////////////////////////////////

void PromotedContentAdEventHandler::GetAdEvents(
    const std::string& placement_id,
    const std::string& creative_instance_id,
    const mojom::PromotedContentAdEventType mojom_ad_event_type,
    FirePromotedContentAdEventHandlerCallback callback) {
  const database::table::AdEvents database_table;
  database_table.Get(
      mojom::AdType::kPromotedContentAd,
      mojom::ConfirmationType::kServedImpression,
      /*time_window=*/base::Days(1),
      base::BindOnce(&PromotedContentAdEventHandler::GetAdEventsCallback,
                     weak_factory_.GetWeakPtr(), placement_id,
                     creative_instance_id, mojom_ad_event_type,
                     std::move(callback)));
}

void PromotedContentAdEventHandler::GetAdEventsCallback(
    const std::string& placement_id,
    const std::string& creative_instance_id,
    const mojom::PromotedContentAdEventType mojom_ad_event_type,
    FirePromotedContentAdEventHandlerCallback callback,
    const bool success,
    const AdEventList& ad_events) {
  if (!success) {
    BLOG(1, "Promoted content ad: Failed to get ad events");
    return FailedToFireEvent(placement_id, creative_instance_id,
                             mojom_ad_event_type, std::move(callback));
  }

  if (mojom_ad_event_type ==
          mojom::PromotedContentAdEventType::kServedImpression &&
      !PromotedContentAdPermissionRules::HasPermission(ad_events)) {
    BLOG(1, "Promoted content ad: Not allowed due to permission rules");
    return FailedToFireEvent(placement_id, creative_instance_id,
                             mojom_ad_event_type, std::move(callback));
  }

  creative_ads_database_table_.GetForCreativeInstanceId(
      creative_instance_id,
      base::BindOnce(
          &PromotedContentAdEventHandler::GetForCreativeInstanceIdCallback,
          weak_factory_.GetWeakPtr(), placement_id, mojom_ad_event_type,
          std::move(callback)));
}

void PromotedContentAdEventHandler::GetForCreativeInstanceIdCallback(
    const std::string& placement_id,
    const mojom::PromotedContentAdEventType mojom_ad_event_type,
    FirePromotedContentAdEventHandlerCallback callback,
    const bool success,
    const std::string& creative_instance_id,
    const CreativePromotedContentAdInfo& creative_ad) {
  if (!success) {
    BLOG(1,
         "Failed to fire promoted content ad event due to missing creative "
         "instance id "
             << creative_instance_id);
    return FailedToFireEvent(placement_id, creative_instance_id,
                             mojom_ad_event_type, std::move(callback));
  }

  const PromotedContentAdInfo ad =
      BuildPromotedContentAd(creative_ad, placement_id);

  ad_events_database_table_.GetUnexpired(
      mojom::AdType::kPromotedContentAd,
      base::BindOnce(&PromotedContentAdEventHandler::GetForTypeCallback,
                     weak_factory_.GetWeakPtr(), ad, mojom_ad_event_type,
                     std::move(callback)));
}

void PromotedContentAdEventHandler::GetForTypeCallback(
    const PromotedContentAdInfo& ad,
    const mojom::PromotedContentAdEventType mojom_ad_event_type,
    FirePromotedContentAdEventHandlerCallback callback,
    const bool success,
    const AdEventList& ad_events) {
  if (!success) {
    BLOG(1, "Promoted content ad: Failed to get ad events");
    return FailedToFireEvent(ad.placement_id, ad.creative_instance_id,
                             mojom_ad_event_type, std::move(callback));
  }

  if (!WasAdServed(ad, ad_events, mojom_ad_event_type)) {
    BLOG(1,
         "Promoted content ad: Not allowed because an ad was not served for "
         "placement id "
             << ad.placement_id);
    return FailedToFireEvent(ad.placement_id, ad.creative_instance_id,
                             mojom_ad_event_type, std::move(callback));
  }

  if (ShouldDeduplicateAdEvent(ad, ad_events, mojom_ad_event_type)) {
    BLOG(1, "Promoted content ad: Not allowed as deduplicated "
                << mojom_ad_event_type << " event for placement id "
                << ad.placement_id);
    return FailedToFireEvent(ad.placement_id, ad.creative_instance_id,
                             mojom_ad_event_type, std::move(callback));
  }

  const auto ad_event =
      PromotedContentAdEventFactory::Build(mojom_ad_event_type);
  ad_event->FireEvent(
      ad, base::BindOnce(&PromotedContentAdEventHandler::FireEventCallback,
                         weak_factory_.GetWeakPtr(), ad, mojom_ad_event_type,
                         std::move(callback)));
}

void PromotedContentAdEventHandler::FireEventCallback(
    const PromotedContentAdInfo& ad,
    const mojom::PromotedContentAdEventType mojom_ad_event_type,
    FirePromotedContentAdEventHandlerCallback callback,
    const bool success) const {
  if (!success) {
    return FailedToFireEvent(ad.placement_id, ad.creative_instance_id,
                             mojom_ad_event_type, std::move(callback));
  }

  SuccessfullyFiredEvent(ad, mojom_ad_event_type, std::move(callback));
}

void PromotedContentAdEventHandler::SuccessfullyFiredEvent(
    const PromotedContentAdInfo& ad,
    const mojom::PromotedContentAdEventType mojom_ad_event_type,
    FirePromotedContentAdEventHandlerCallback callback) const {
  NotifyDidFirePromotedContentAdEvent(ad, mojom_ad_event_type);

  std::move(callback).Run(/*success=*/true, ad.placement_id,
                          mojom_ad_event_type);
}

void PromotedContentAdEventHandler::FailedToFireEvent(
    const std::string& placement_id,
    const std::string& creative_instance_id,
    const mojom::PromotedContentAdEventType mojom_ad_event_type,
    FirePromotedContentAdEventHandlerCallback callback) const {
  BLOG(1, "Failed to fire promoted content ad "
              << mojom_ad_event_type << " event for placement id "
              << placement_id << " and creative instance id "
              << creative_instance_id);

  NotifyFailedToFirePromotedContentAdEvent(placement_id, creative_instance_id,
                                           mojom_ad_event_type);

  std::move(callback).Run(/*success=*/false, placement_id, mojom_ad_event_type);
}

void PromotedContentAdEventHandler::NotifyDidFirePromotedContentAdEvent(
    const PromotedContentAdInfo& ad,
    mojom::PromotedContentAdEventType mojom_ad_event_type) const {
  if (!delegate_) {
    return;
  }

  switch (mojom_ad_event_type) {
    case mojom::PromotedContentAdEventType::kServedImpression: {
      delegate_->OnDidFirePromotedContentAdServedEvent(ad);
      break;
    }

    case mojom::PromotedContentAdEventType::kViewedImpression: {
      delegate_->OnDidFirePromotedContentAdViewedEvent(ad);
      break;
    }

    case mojom::PromotedContentAdEventType::kClicked: {
      delegate_->OnDidFirePromotedContentAdClickedEvent(ad);
      break;
    }
  }
}

void PromotedContentAdEventHandler::NotifyFailedToFirePromotedContentAdEvent(
    const std::string& placement_id,
    const std::string& creative_instance_id,
    const mojom::PromotedContentAdEventType mojom_ad_event_type) const {
  if (delegate_) {
    delegate_->OnFailedToFirePromotedContentAdEvent(
        placement_id, creative_instance_id, mojom_ad_event_type);
  }
}

}  // namespace brave_ads
