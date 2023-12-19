/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/user_engagement/ad_events/inline_content_ads/inline_content_ad_event_handler.h"

#include <utility>

#include "base/functional/bind.h"
#include "brave/components/brave_ads/core/internal/common/logging_util.h"
#include "brave/components/brave_ads/core/internal/creatives/inline_content_ads/creative_inline_content_ad_info.h"
#include "brave/components/brave_ads/core/internal/creatives/inline_content_ads/creative_inline_content_ads_database_table.h"
#include "brave/components/brave_ads/core/internal/creatives/inline_content_ads/inline_content_ad_builder.h"
#include "brave/components/brave_ads/core/internal/user_engagement/ad_events/ad_event_handler_util.h"
#include "brave/components/brave_ads/core/internal/user_engagement/ad_events/ad_events_database_table.h"
#include "brave/components/brave_ads/core/internal/user_engagement/ad_events/inline_content_ads/inline_content_ad_event_factory.h"
#include "brave/components/brave_ads/core/mojom/brave_ads.mojom-shared.h"
#include "brave/components/brave_ads/core/public/ad_units/inline_content_ad/inline_content_ad_info.h"

namespace brave_ads {

InlineContentAdEventHandler::InlineContentAdEventHandler() = default;

InlineContentAdEventHandler::~InlineContentAdEventHandler() {
  delegate_ = nullptr;
}

void InlineContentAdEventHandler::FireEvent(
    const std::string& placement_id,
    const std::string& creative_instance_id,
    const mojom::InlineContentAdEventType event_type,
    FireInlineContentAdEventHandlerCallback callback) {
  if (placement_id.empty()) {
    BLOG(1,
         "Failed to fire inline content ad event due to an invalid placement "
         "id");
    return FailedToFireEvent(placement_id, creative_instance_id, event_type,
                             std::move(callback));
  }

  if (creative_instance_id.empty()) {
    BLOG(1,
         "Failed to fire inline content ad event due to an invalid creative "
         "instance id");
    return FailedToFireEvent(placement_id, creative_instance_id, event_type,
                             std::move(callback));
  }

  const database::table::CreativeInlineContentAds database_table;
  database_table.GetForCreativeInstanceId(
      creative_instance_id,
      base::BindOnce(
          &InlineContentAdEventHandler::GetForCreativeInstanceIdCallback,
          weak_factory_.GetWeakPtr(), placement_id, event_type,
          std::move(callback)));
}

///////////////////////////////////////////////////////////////////////////////

void InlineContentAdEventHandler::GetForCreativeInstanceIdCallback(
    const std::string& placement_id,
    const mojom::InlineContentAdEventType event_type,
    FireInlineContentAdEventHandlerCallback callback,
    const bool success,
    const std::string& creative_instance_id,
    const CreativeInlineContentAdInfo& creative_ad) {
  if (!success) {
    BLOG(1,
         "Failed to fire inline content ad event due to missing creative "
         "instance id "
             << creative_instance_id);
    return FailedToFireEvent(placement_id, creative_instance_id, event_type,
                             std::move(callback));
  }

  const InlineContentAdInfo ad =
      BuildInlineContentAd(creative_ad, placement_id);

  const database::table::AdEvents database_table;
  database_table.GetForType(
      mojom::AdType::kInlineContentAd,
      base::BindOnce(&InlineContentAdEventHandler::GetForTypeCallback,
                     weak_factory_.GetWeakPtr(), ad, event_type,
                     std::move(callback)));
}

void InlineContentAdEventHandler::GetForTypeCallback(
    const InlineContentAdInfo& ad,
    const mojom::InlineContentAdEventType event_type,
    FireInlineContentAdEventHandlerCallback callback,
    const bool success,
    const AdEventList& ad_events) {
  if (!success) {
    BLOG(1, "Inline content ad: Failed to get ad events");
    return FailedToFireEvent(ad.placement_id, ad.creative_instance_id,
                             event_type, std::move(callback));
  }

  if (!WasAdServed(ad, ad_events, event_type)) {
    BLOG(1,
         "Inline content ad: Not allowed because an ad was not served for "
         "placement id "
             << ad.placement_id);
    return FailedToFireEvent(ad.placement_id, ad.creative_instance_id,
                             event_type, std::move(callback));
  }

  if (ShouldDebounceAdEvent(ad, ad_events, event_type)) {
    BLOG(1, "Inline content ad: Not allowed as debounced "
                << event_type << " event for placement id " << ad.placement_id);
    return FailedToFireEvent(ad.placement_id, ad.creative_instance_id,
                             event_type, std::move(callback));
  }

  const auto ad_event = InlineContentAdEventFactory::Build(event_type);
  ad_event->FireEvent(
      ad, base::BindOnce(&InlineContentAdEventHandler::FireEventCallback,
                         weak_factory_.GetWeakPtr(), ad, event_type,
                         std::move(callback)));
}

void InlineContentAdEventHandler::FireEventCallback(
    const InlineContentAdInfo& ad,
    const mojom::InlineContentAdEventType event_type,
    FireInlineContentAdEventHandlerCallback callback,
    const bool success) const {
  if (!success) {
    return FailedToFireEvent(ad.placement_id, ad.creative_instance_id,
                             event_type, std::move(callback));
  }

  SuccessfullyFiredEvent(ad, event_type, std::move(callback));
}

void InlineContentAdEventHandler::SuccessfullyFiredEvent(
    const InlineContentAdInfo& ad,
    const mojom::InlineContentAdEventType event_type,
    FireInlineContentAdEventHandlerCallback callback) const {
  NotifyDidFireInlineContentAdEvent(ad, event_type);

  std::move(callback).Run(/*success=*/true, ad.placement_id, event_type);
}

void InlineContentAdEventHandler::FailedToFireEvent(
    const std::string& placement_id,
    const std::string& creative_instance_id,
    const mojom::InlineContentAdEventType event_type,
    FireInlineContentAdEventHandlerCallback callback) const {
  BLOG(1, "Failed to fire inline content ad "
              << event_type << " event for placement id " << placement_id
              << " and creative instance id " << creative_instance_id);

  NotifyFailedToFireInlineContentAdEvent(placement_id, creative_instance_id,
                                         event_type);

  std::move(callback).Run(/*success=*/false, placement_id, event_type);
}

void InlineContentAdEventHandler::NotifyDidFireInlineContentAdEvent(
    const InlineContentAdInfo& ad,
    mojom::InlineContentAdEventType event_type) const {
  if (!delegate_) {
    return;
  }

  switch (event_type) {
    case mojom::InlineContentAdEventType::kServed: {
      delegate_->OnDidFireInlineContentAdServedEvent(ad);
      break;
    }

    case mojom::InlineContentAdEventType::kViewed: {
      delegate_->OnDidFireInlineContentAdViewedEvent(ad);
      break;
    }

    case mojom::InlineContentAdEventType::kClicked: {
      delegate_->OnDidFireInlineContentAdClickedEvent(ad);
      break;
    }
  }
}

void InlineContentAdEventHandler::NotifyFailedToFireInlineContentAdEvent(
    const std::string& placement_id,
    const std::string& creative_instance_id,
    const mojom::InlineContentAdEventType event_type) const {
  if (delegate_) {
    delegate_->OnFailedToFireInlineContentAdEvent(
        placement_id, creative_instance_id, event_type);
  }
}

}  // namespace brave_ads
