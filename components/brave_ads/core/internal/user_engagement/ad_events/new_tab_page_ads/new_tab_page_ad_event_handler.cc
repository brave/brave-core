/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/user_engagement/ad_events/new_tab_page_ads/new_tab_page_ad_event_handler.h"

#include <utility>

#include "base/debug/crash_logging.h"
#include "base/functional/bind.h"
#include "base/notreached.h"
#include "brave/components/brave_ads/core/internal/common/logging_util.h"
#include "brave/components/brave_ads/core/internal/creatives/new_tab_page_ads/creative_new_tab_page_ad_info.h"
#include "brave/components/brave_ads/core/internal/creatives/new_tab_page_ads/new_tab_page_ad_builder.h"
#include "brave/components/brave_ads/core/internal/user_engagement/ad_events/new_tab_page_ads/new_tab_page_ad_event_factory.h"
#include "brave/components/brave_ads/core/internal/user_engagement/ad_events/new_tab_page_ads/new_tab_page_ad_event_handler_util.h"
#include "brave/components/brave_ads/core/public/ad_units/new_tab_page_ad/new_tab_page_ad_info.h"

namespace brave_ads {

NewTabPageAdEventHandler::NewTabPageAdEventHandler() = default;

NewTabPageAdEventHandler::~NewTabPageAdEventHandler() {
  delegate_ = nullptr;
}

void NewTabPageAdEventHandler::FireEvent(
    const std::string& placement_id,
    const std::string& creative_instance_id,
    mojom::NewTabPageAdEventType mojom_ad_event_type,
    FireNewTabPageAdEventHandlerCallback callback) {
  if (placement_id.empty()) {
    BLOG(0,
         "Failed to fire new tab page ad event due to an invalid placement id");
    return FailedToFireEvent(placement_id, creative_instance_id,
                             mojom_ad_event_type, std::move(callback));
  }

  if (creative_instance_id.empty()) {
    return FailedToFireEvent(placement_id, creative_instance_id,
                             mojom_ad_event_type, std::move(callback));
  }

  if (!IsAllowedToFireAdEvent()) {
    BLOG(1, "New tab page ad: Not allowed to fire event");
    return FailedToFireEvent(placement_id, creative_instance_id,
                             mojom_ad_event_type, std::move(callback));
  }

  creative_ads_database_table_.GetForCreativeInstanceId(
      creative_instance_id,
      base::BindOnce(&NewTabPageAdEventHandler::GetCreativeAdCallback,
                     weak_factory_.GetWeakPtr(), placement_id,
                     mojom_ad_event_type, std::move(callback)));
}

///////////////////////////////////////////////////////////////////////////////

void NewTabPageAdEventHandler::GetCreativeAdCallback(
    const std::string& placement_id,
    mojom::NewTabPageAdEventType mojom_ad_event_type,
    FireNewTabPageAdEventHandlerCallback callback,
    bool success,
    const std::string& creative_instance_id,
    const CreativeNewTabPageAdInfo& creative_ad) {
  if (!success) {
    BLOG(0,
         "Failed to fire new tab page ad event due to missing creative "
         "instance id "
             << creative_instance_id);
    return FailedToFireEvent(placement_id, creative_instance_id,
                             mojom_ad_event_type, std::move(callback));
  }

  const NewTabPageAdInfo ad = BuildNewTabPageAd(placement_id, creative_ad);
  if (!ad.IsValid()) {
    BLOG(0, "Failed to fire new tab page ad event due to the ad being invalid");
    SCOPED_CRASH_KEY_NUMBER("Issue50267", "event_type",
                            static_cast<int>(mojom_ad_event_type));
    SCOPED_CRASH_KEY_STRING64("Issue50267", "creative_instance_id",
                              creative_instance_id);
    SCOPED_CRASH_KEY_STRING64("Issue50267", "failure_reason", "Invalid ad");
    DUMP_WILL_BE_NOTREACHED();
    return FailedToFireEvent(placement_id, creative_instance_id,
                             mojom_ad_event_type, std::move(callback));
  }

  ad_events_database_table_.GetUnexpired(
      mojom::AdType::kNewTabPageAd,
      base::BindOnce(&NewTabPageAdEventHandler::GetAdEventsCallback,
                     weak_factory_.GetWeakPtr(), ad, mojom_ad_event_type,
                     std::move(callback)));
}

void NewTabPageAdEventHandler::GetAdEventsCallback(
    const NewTabPageAdInfo& ad,
    mojom::NewTabPageAdEventType mojom_ad_event_type,
    FireNewTabPageAdEventHandlerCallback callback,
    bool success,
    const AdEventList& ad_events) {
  if (!success) {
    BLOG(1, "New tab page ad: Failed to get ad events");
    SCOPED_CRASH_KEY_NUMBER("Issue50267", "event_type",
                            static_cast<int>(mojom_ad_event_type));
    SCOPED_CRASH_KEY_STRING64("Issue50267", "creative_instance_id",
                              ad.creative_instance_id);
    SCOPED_CRASH_KEY_STRING64("Issue50267", "failure_reason",
                              "Failed to get ad events");
    DUMP_WILL_BE_NOTREACHED();
    return FailedToFireEvent(ad.placement_id, ad.creative_instance_id,
                             mojom_ad_event_type, std::move(callback));
  }

  if (!ShouldFireAdEvent(ad, ad_events, mojom_ad_event_type)) {
    return FailedToFireEvent(ad.placement_id, ad.creative_instance_id,
                             mojom_ad_event_type, std::move(callback));
  }

  FireEvent(ad, mojom_ad_event_type, std::move(callback));
}

void NewTabPageAdEventHandler::FireEvent(
    const NewTabPageAdInfo& ad,
    mojom::NewTabPageAdEventType mojom_ad_event_type,
    FireNewTabPageAdEventHandlerCallback callback) const {
  const auto ad_event = NewTabPageAdEventFactory::Build(mojom_ad_event_type);
  ad_event->FireEvent(
      ad, base::BindOnce(&NewTabPageAdEventHandler::FireEventCallback,
                         weak_factory_.GetWeakPtr(), ad, mojom_ad_event_type,
                         std::move(callback)));
}

void NewTabPageAdEventHandler::FireEventCallback(
    const NewTabPageAdInfo& ad,
    mojom::NewTabPageAdEventType mojom_ad_event_type,
    FireNewTabPageAdEventHandlerCallback callback,
    bool success) const {
  if (!success) {
    SCOPED_CRASH_KEY_NUMBER("Issue50267", "event_type",
                            static_cast<int>(mojom_ad_event_type));
    SCOPED_CRASH_KEY_STRING64("Issue50267", "creative_instance_id",
                              ad.creative_instance_id);
    SCOPED_CRASH_KEY_STRING64("Issue50267", "failure_reason",
                              "Failed to fire ad event");
    DUMP_WILL_BE_NOTREACHED();
    return FailedToFireEvent(ad.placement_id, ad.creative_instance_id,
                             mojom_ad_event_type, std::move(callback));
  }

  SuccessfullyFiredEvent(ad, mojom_ad_event_type, std::move(callback));
}

void NewTabPageAdEventHandler::SuccessfullyFiredEvent(
    const NewTabPageAdInfo& ad,
    mojom::NewTabPageAdEventType mojom_ad_event_type,
    FireNewTabPageAdEventHandlerCallback callback) const {
  NotifyDidFireNewTabPageAdEvent(ad, mojom_ad_event_type);

  std::move(callback).Run(/*success=*/true, ad.placement_id,
                          mojom_ad_event_type);
}

void NewTabPageAdEventHandler::FailedToFireEvent(
    const std::string& placement_id,
    const std::string& creative_instance_id,
    mojom::NewTabPageAdEventType mojom_ad_event_type,
    FireNewTabPageAdEventHandlerCallback callback) const {
  BLOG(0, "Failed to fire new tab page ad "
              << mojom_ad_event_type << " event for placement id "
              << placement_id << " and creative instance id "
              << creative_instance_id);

  NotifyFailedToFireNewTabPageAdEvent(placement_id, creative_instance_id,
                                      mojom_ad_event_type);

  std::move(callback).Run(/*success=*/false, placement_id, mojom_ad_event_type);
}

void NewTabPageAdEventHandler::NotifyDidFireNewTabPageAdEvent(
    const NewTabPageAdInfo& ad,
    mojom::NewTabPageAdEventType mojom_ad_event_type) const {
  if (!delegate_) {
    return;
  }

  switch (mojom_ad_event_type) {
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

    case mojom::NewTabPageAdEventType::kInteraction: {
      delegate_->OnDidFireNewTabPageAdInteractionEvent(ad);
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
    mojom::NewTabPageAdEventType mojom_ad_event_type) const {
  if (delegate_) {
    delegate_->OnFailedToFireNewTabPageAdEvent(
        placement_id, creative_instance_id, mojom_ad_event_type);
  }
}

}  // namespace brave_ads
