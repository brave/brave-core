/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/user/user_interaction/ad_events/promoted_content_ads/promoted_content_ad_event_handler.h"

#include <utility>

#include "base/check.h"
#include "base/functional/bind.h"
#include "brave/components/brave_ads/core/internal/common/logging_util.h"
#include "brave/components/brave_ads/core/internal/creatives/promoted_content_ads/creative_promoted_content_ad_info.h"
#include "brave/components/brave_ads/core/internal/creatives/promoted_content_ads/creative_promoted_content_ads_database_table.h"
#include "brave/components/brave_ads/core/internal/creatives/promoted_content_ads/promoted_content_ad_builder.h"
#include "brave/components/brave_ads/core/internal/serving/permission_rules/promoted_content_ads/promoted_content_ad_permission_rules.h"
#include "brave/components/brave_ads/core/internal/units/promoted_content_ad/promoted_content_ad_info.h"
#include "brave/components/brave_ads/core/internal/user/user_interaction/ad_events/ad_event_handler_util.h"
#include "brave/components/brave_ads/core/internal/user/user_interaction/ad_events/ad_events_database_table.h"
#include "brave/components/brave_ads/core/internal/user/user_interaction/ad_events/promoted_content_ads/promoted_content_ad_event_factory.h"

namespace brave_ads {

PromotedContentAdEventHandler::PromotedContentAdEventHandler() = default;

PromotedContentAdEventHandler::~PromotedContentAdEventHandler() {
  delegate_ = nullptr;
}

void PromotedContentAdEventHandler::FireEvent(
    const std::string& placement_id,
    const std::string& creative_instance_id,
    const mojom::PromotedContentAdEventType event_type,
    FirePromotedContentAdEventHandlerCallback callback) {
  CHECK(mojom::IsKnownEnumValue(event_type));

  if (placement_id.empty()) {
    BLOG(1,
         "Failed to fire promoted content ad event due to an invalid placement "
         "id");
    return FailedToFireEvent(placement_id, creative_instance_id, event_type,
                             std::move(callback));
  }

  if (creative_instance_id.empty()) {
    BLOG(1,
         "Failed to fire promoted content ad event due to an invalid creative "
         "instance id");
    return FailedToFireEvent(placement_id, creative_instance_id, event_type,
                             std::move(callback));
  }

  if (event_type == mojom::PromotedContentAdEventType::kServed &&
      !PromotedContentAdPermissionRules::HasPermission()) {
    BLOG(1, "Promoted content ad: Not allowed due to permission rules");
    return FailedToFireEvent(placement_id, creative_instance_id, event_type,
                             std::move(callback));
  }

  const database::table::CreativePromotedContentAds database_table;
  database_table.GetForCreativeInstanceId(
      creative_instance_id,
      base::BindOnce(
          &PromotedContentAdEventHandler::GetForCreativeInstanceIdCallback,
          weak_factory_.GetWeakPtr(), placement_id, event_type,
          std::move(callback)));
}

///////////////////////////////////////////////////////////////////////////////

void PromotedContentAdEventHandler::GetForCreativeInstanceIdCallback(
    const std::string& placement_id,
    const mojom::PromotedContentAdEventType event_type,
    FirePromotedContentAdEventHandlerCallback callback,
    const bool success,
    const std::string& creative_instance_id,
    const CreativePromotedContentAdInfo& creative_ad) {
  CHECK(mojom::IsKnownEnumValue(event_type));

  if (!success) {
    BLOG(1,
         "Failed to fire promoted content ad event due to missing creative "
         "instance id "
             << creative_instance_id);
    return FailedToFireEvent(placement_id, creative_instance_id, event_type,
                             std::move(callback));
  }

  const PromotedContentAdInfo ad =
      BuildPromotedContentAd(creative_ad, placement_id);

  const database::table::AdEvents database_table;
  database_table.GetForType(
      mojom::AdType::kPromotedContentAd,
      base::BindOnce(&PromotedContentAdEventHandler::GetForTypeCallback,
                     weak_factory_.GetWeakPtr(), ad, event_type,
                     std::move(callback)));
}

void PromotedContentAdEventHandler::GetForTypeCallback(
    const PromotedContentAdInfo& ad,
    const mojom::PromotedContentAdEventType event_type,
    FirePromotedContentAdEventHandlerCallback callback,
    const bool success,
    const AdEventList& ad_events) {
  if (!success) {
    BLOG(1, "Promoted content ad: Failed to get ad events");
    return FailedToFireEvent(ad.placement_id, ad.creative_instance_id,
                             event_type, std::move(callback));
  }

  if (!WasAdServed(ad, ad_events, event_type)) {
    BLOG(1,
         "Promoted content ad: Not allowed because an ad was not served for "
         "placement id "
             << ad.placement_id);
    return FailedToFireEvent(ad.placement_id, ad.creative_instance_id,
                             event_type, std::move(callback));
  }

  if (ShouldDebounceAdEvent(ad, ad_events, event_type)) {
    BLOG(1, "Promoted content ad: Not allowed as debounced "
                << event_type << " event for placement id " << ad.placement_id);
    return FailedToFireEvent(ad.placement_id, ad.creative_instance_id,
                             event_type, std::move(callback));
  }

  const auto ad_event = PromotedContentAdEventFactory::Build(event_type);
  ad_event->FireEvent(
      ad, base::BindOnce(&PromotedContentAdEventHandler::FireEventCallback,
                         weak_factory_.GetWeakPtr(), ad, event_type,
                         std::move(callback)));
}

void PromotedContentAdEventHandler::FireEventCallback(
    const PromotedContentAdInfo& ad,
    const mojom::PromotedContentAdEventType event_type,
    FirePromotedContentAdEventHandlerCallback callback,
    const bool success) const {
  if (!success) {
    return FailedToFireEvent(ad.placement_id, ad.creative_instance_id,
                             event_type, std::move(callback));
  }

  SuccessfullyFiredEvent(ad, event_type, std::move(callback));
}

void PromotedContentAdEventHandler::SuccessfullyFiredEvent(
    const PromotedContentAdInfo& ad,
    const mojom::PromotedContentAdEventType event_type,
    FirePromotedContentAdEventHandlerCallback callback) const {
  CHECK(mojom::IsKnownEnumValue(event_type));

  NotifyDidFirePromotedContentAdEvent(ad, event_type);

  std::move(callback).Run(/*success=*/true, ad.placement_id, event_type);
}

void PromotedContentAdEventHandler::FailedToFireEvent(
    const std::string& placement_id,
    const std::string& creative_instance_id,
    const mojom::PromotedContentAdEventType event_type,
    FirePromotedContentAdEventHandlerCallback callback) const {
  CHECK(mojom::IsKnownEnumValue(event_type));

  BLOG(1, "Failed to fire promoted content ad "
              << event_type << " event for placement id " << placement_id
              << " and creative instance id " << creative_instance_id);

  NotifyFailedToFirePromotedContentAdEvent(placement_id, creative_instance_id,
                                           event_type);

  std::move(callback).Run(/*success=*/false, placement_id, event_type);
}

void PromotedContentAdEventHandler::NotifyDidFirePromotedContentAdEvent(
    const PromotedContentAdInfo& ad,
    mojom::PromotedContentAdEventType event_type) const {
  if (!delegate_) {
    return;
  }

  switch (event_type) {
    case mojom::PromotedContentAdEventType::kServed: {
      delegate_->OnDidFirePromotedContentAdServedEvent(ad);
      break;
    }

    case mojom::PromotedContentAdEventType::kViewed: {
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
    const mojom::PromotedContentAdEventType event_type) const {
  if (delegate_) {
    delegate_->OnFailedToFirePromotedContentAdEvent(
        placement_id, creative_instance_id, event_type);
  }
}

}  // namespace brave_ads
