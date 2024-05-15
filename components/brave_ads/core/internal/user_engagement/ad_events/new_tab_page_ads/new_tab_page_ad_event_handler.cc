/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/user_engagement/ad_events/new_tab_page_ads/new_tab_page_ad_event_handler.h"

#include <utility>

#include "base/functional/bind.h"
#include "brave/components/brave_ads/core/internal/common/logging_util.h"
#include "brave/components/brave_ads/core/internal/creatives/new_tab_page_ads/creative_new_tab_page_ad_info.h"
#include "brave/components/brave_ads/core/internal/creatives/new_tab_page_ads/new_tab_page_ad_builder.h"
#include "brave/components/brave_ads/core/internal/user_engagement/ad_events/ad_event_handler_util.h"
#include "brave/components/brave_ads/core/internal/user_engagement/ad_events/new_tab_page_ads/new_tab_page_ad_event_factory.h"
#include "brave/components/brave_ads/core/public/ad_units/new_tab_page_ad/new_tab_page_ad_info.h"

namespace brave_ads {

NewTabPageAdEventHandler::NewTabPageAdEventHandler() = default;

NewTabPageAdEventHandler::~NewTabPageAdEventHandler() {
  delegate_ = nullptr;
}

void NewTabPageAdEventHandler::FireEvent(
    const std::string& placement_id,
    const std::string& creative_instance_id,
    const mojom::NewTabPageAdEventType event_type,
    FireNewTabPageAdEventHandlerCallback callback) {
  if (placement_id.empty()) {
    BLOG(1,
         "Failed to fire new tab page ad event due to an invalid placement id");
    return FailedToFireEvent(placement_id, creative_instance_id, event_type,
                             std::move(callback));
  }

  if (creative_instance_id.empty()) {
    BLOG(1,
         "Failed to fire new tab page ad event due to an invalid creative "
         "instance id");
    return FailedToFireEvent(placement_id, creative_instance_id, event_type,
                             std::move(callback));
  }

  database_table_.GetForCreativeInstanceId(
      creative_instance_id,
      base::BindOnce(
          &NewTabPageAdEventHandler::GetForCreativeInstanceIdCallback,
          weak_factory_.GetWeakPtr(), placement_id, event_type,
          std::move(callback)));
}

///////////////////////////////////////////////////////////////////////////////

void NewTabPageAdEventHandler::GetForCreativeInstanceIdCallback(
    const std::string& placement_id,
    const mojom::NewTabPageAdEventType event_type,
    FireNewTabPageAdEventHandlerCallback callback,
    const bool success,
    const std::string& creative_instance_id,
    const CreativeNewTabPageAdInfo& creative_ad) {
  if (!success) {
    BLOG(1,
         "Failed to fire new tab page ad event due to missing creative "
         "instance id "
             << creative_instance_id);
    return FailedToFireEvent(placement_id, creative_instance_id, event_type,
                             std::move(callback));
  }

  const NewTabPageAdInfo ad = BuildNewTabPageAd(creative_ad, placement_id);

  ad_events_database_table_.GetUnexpiredForType(
      mojom::AdType::kNewTabPageAd,
      base::BindOnce(&NewTabPageAdEventHandler::GetForTypeCallback,
                     weak_factory_.GetWeakPtr(), ad, event_type,
                     std::move(callback)));
}

void NewTabPageAdEventHandler::GetForTypeCallback(
    const NewTabPageAdInfo& ad,
    const mojom::NewTabPageAdEventType event_type,
    FireNewTabPageAdEventHandlerCallback callback,
    const bool success,
    const AdEventList& ad_events) {
  if (!success) {
    return FailedToFireEvent(ad.placement_id, ad.creative_instance_id,
                             event_type, std::move(callback));
  }

  if (!WasAdServed(ad, ad_events, event_type)) {
    BLOG(1,
         "New tab page ad: Not allowed because an ad was not served for "
         "placement id "
             << ad.placement_id);
    return FailedToFireEvent(ad.placement_id, ad.creative_instance_id,
                             event_type, std::move(callback));
  }

  if (ShouldDeduplicateAdEvent(ad, ad_events, event_type)) {
    BLOG(1, "New tab page ad: Not allowed as deduplicated "
                << event_type << " event for placement id " << ad.placement_id);
    return FailedToFireEvent(ad.placement_id, ad.creative_instance_id,
                             event_type, std::move(callback));
  }

  const auto ad_event = NewTabPageAdEventFactory::Build(event_type);
  ad_event->FireEvent(
      ad, base::BindOnce(&NewTabPageAdEventHandler::FireEventCallback,
                         weak_factory_.GetWeakPtr(), ad, event_type,
                         std::move(callback)));
}

void NewTabPageAdEventHandler::FireEventCallback(
    const NewTabPageAdInfo& ad,
    const mojom::NewTabPageAdEventType event_type,
    FireNewTabPageAdEventHandlerCallback callback,
    const bool success) const {
  if (!success) {
    return FailedToFireEvent(ad.placement_id, ad.creative_instance_id,
                             event_type, std::move(callback));
  }

  SuccessfullyFiredEvent(ad, event_type, std::move(callback));
}

void NewTabPageAdEventHandler::SuccessfullyFiredEvent(
    const NewTabPageAdInfo& ad,
    const mojom::NewTabPageAdEventType event_type,
    FireNewTabPageAdEventHandlerCallback callback) const {
  NotifyDidFireNewTabPageAdEvent(ad, event_type);

  std::move(callback).Run(/*success=*/true, ad.placement_id, event_type);
}

void NewTabPageAdEventHandler::FailedToFireEvent(
    const std::string& placement_id,
    const std::string& creative_instance_id,
    const mojom::NewTabPageAdEventType event_type,
    FireNewTabPageAdEventHandlerCallback callback) const {
  BLOG(1, "Failed to fire new tab page ad "
              << event_type << " event for placement id " << placement_id
              << " and creative instance id " << creative_instance_id);

  NotifyFailedToFireNewTabPageAdEvent(placement_id, creative_instance_id,
                                      event_type);

  std::move(callback).Run(/*success=*/false, placement_id, event_type);
}

void NewTabPageAdEventHandler::NotifyDidFireNewTabPageAdEvent(
    const NewTabPageAdInfo& ad,
    mojom::NewTabPageAdEventType event_type) const {
  if (!delegate_) {
    return;
  }

  switch (event_type) {
    case mojom::NewTabPageAdEventType::kServedImpression: {
      delegate_->OnDidFireNewTabPageAdServedEvent(ad);
      break;
    }

    case mojom::NewTabPageAdEventType::kViewedImpression: {
      delegate_->OnDidFireNewTabPageAdViewedEvent(ad);
      break;
    }

    case mojom::NewTabPageAdEventType::kClicked: {
      delegate_->OnDidFireNewTabPageAdClickedEvent(ad);
      break;
    }

    case mojom::NewTabPageAdEventType::kMediaPlay: {
      delegate_->OnDidFireNewTabPageAdMediaPlayEvent(ad);
      break;
    }

    case mojom::NewTabPageAdEventType::kMedia25: {
      delegate_->OnDidFireNewTabPageAdMedia25Event(ad);
      break;
    }

    case mojom::NewTabPageAdEventType::kMedia100: {
      delegate_->OnDidFireNewTabPageAdMedia100Event(ad);
      break;
    }
  }
}

void NewTabPageAdEventHandler::NotifyFailedToFireNewTabPageAdEvent(
    const std::string& placement_id,
    const std::string& creative_instance_id,
    const mojom::NewTabPageAdEventType event_type) const {
  if (delegate_) {
    delegate_->OnFailedToFireNewTabPageAdEvent(
        placement_id, creative_instance_id, event_type);
  }
}

}  // namespace brave_ads
