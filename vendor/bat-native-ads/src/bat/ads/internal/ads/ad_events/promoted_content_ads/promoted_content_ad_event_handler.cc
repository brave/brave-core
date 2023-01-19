/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/ads/ad_events/promoted_content_ads/promoted_content_ad_event_handler.h"

#include "base/check.h"
#include "base/functional/bind.h"
#include "bat/ads/internal/ads/ad_events/ad_event_util.h"
#include "bat/ads/internal/ads/ad_events/ad_events_database_table.h"
#include "bat/ads/internal/ads/ad_events/promoted_content_ads/promoted_content_ad_event_factory.h"
#include "bat/ads/internal/ads/serving/permission_rules/promoted_content_ads/promoted_content_ad_permission_rules.h"
#include "bat/ads/internal/common/logging_util.h"
#include "bat/ads/internal/creatives/promoted_content_ads/creative_promoted_content_ad_info.h"
#include "bat/ads/internal/creatives/promoted_content_ads/creative_promoted_content_ads_database_table.h"
#include "bat/ads/internal/creatives/promoted_content_ads/promoted_content_ad_builder.h"
#include "bat/ads/promoted_content_ad_info.h"

namespace ads::promoted_content_ads {

namespace {

bool ShouldDebounceViewedAdEvent(
    const AdInfo& ad,
    const AdEventList& ad_events,
    const mojom::PromotedContentAdEventType& event_type) {
  DCHECK(mojom::IsKnownEnumValue(event_type));

  return event_type == mojom::PromotedContentAdEventType::kViewed &&
         HasFiredAdEvent(ad, ad_events, ConfirmationType::kViewed);
}

bool ShouldDebounceClickedAdEvent(
    const AdInfo& ad,
    const AdEventList& ad_events,
    const mojom::PromotedContentAdEventType& event_type) {
  DCHECK(mojom::IsKnownEnumValue(event_type));

  return event_type == mojom::PromotedContentAdEventType::kClicked &&
         HasFiredAdEvent(ad, ad_events, ConfirmationType::kClicked);
}

bool WasAdServed(const AdInfo& ad,
                 const AdEventList& ad_events,
                 const mojom::PromotedContentAdEventType& event_type) {
  DCHECK(mojom::IsKnownEnumValue(event_type));

  return event_type == mojom::PromotedContentAdEventType::kServed ||
         HasFiredAdEvent(ad, ad_events, ConfirmationType::kServed);
}

bool IsAdPlaced(const AdInfo& ad,
                const AdEventList& ad_events,
                const mojom::PromotedContentAdEventType& event_type) {
  DCHECK(mojom::IsKnownEnumValue(event_type));

  return event_type == mojom::PromotedContentAdEventType::kServed ||
         event_type == mojom::PromotedContentAdEventType::kViewed ||
         (HasFiredAdEvent(ad, ad_events, ConfirmationType::kServed) &&
          HasFiredAdEvent(ad, ad_events, ConfirmationType::kViewed));
}

bool ShouldDebounceAdEvent(
    const AdInfo& ad,
    const AdEventList& ad_events,
    const mojom::PromotedContentAdEventType& event_type) {
  DCHECK(mojom::IsKnownEnumValue(event_type));

  return ShouldDebounceViewedAdEvent(ad, ad_events, event_type) ||
         ShouldDebounceClickedAdEvent(ad, ad_events, event_type) ||
         !IsAdPlaced(ad, ad_events, event_type);
}

}  // namespace

EventHandler::EventHandler() = default;

EventHandler::~EventHandler() = default;

void EventHandler::AddObserver(EventHandlerObserver* observer) {
  DCHECK(observer);
  observers_.AddObserver(observer);
}

void EventHandler::RemoveObserver(EventHandlerObserver* observer) {
  DCHECK(observer);
  observers_.RemoveObserver(observer);
}

void EventHandler::FireEvent(
    const std::string& placement_id,
    const std::string& creative_instance_id,
    const mojom::PromotedContentAdEventType event_type) {
  DCHECK(mojom::IsKnownEnumValue(event_type));

  if (placement_id.empty()) {
    BLOG(1,
         "Failed to fire promoted content ad event due to an invalid placement "
         "id");
    return FailedToFireEvent(placement_id, creative_instance_id, event_type);
  }

  if (creative_instance_id.empty()) {
    BLOG(1,
         "Failed to fire promoted content ad event due to an invalid creative "
         "instance id");
    return FailedToFireEvent(placement_id, creative_instance_id, event_type);
  }

  if (event_type == mojom::PromotedContentAdEventType::kServed &&
      !PermissionRules::HasPermission()) {
    BLOG(1, "Promoted content ad: Not allowed due to permission rules");
    return FailedToFireEvent(placement_id, creative_instance_id, event_type);
  }

  const database::table::CreativePromotedContentAds database_table;
  database_table.GetForCreativeInstanceId(
      creative_instance_id,
      base::BindOnce(&EventHandler::OnGetForCreativeInstanceId,
                     base::Unretained(this), placement_id, event_type));
}

void EventHandler::OnGetForCreativeInstanceId(
    const std::string& placement_id,
    const mojom::PromotedContentAdEventType event_type,
    const bool success,
    const std::string& creative_instance_id,
    const CreativePromotedContentAdInfo& creative_ad) {
  if (!success) {
    BLOG(1,
         "Failed to fire promoted content ad event due to missing "
         "creative instance id "
             << creative_instance_id);
    return FailedToFireEvent(placement_id, creative_instance_id, event_type);
  }

  const PromotedContentAdInfo ad =
      BuildPromotedContentAd(creative_ad, placement_id);
  FireEvent(ad, event_type);
}

///////////////////////////////////////////////////////////////////////////////

void EventHandler::FireEvent(
    const PromotedContentAdInfo& ad,
    const mojom::PromotedContentAdEventType event_type) {
  DCHECK(mojom::IsKnownEnumValue(event_type));

  const database::table::AdEvents database_table;
  database_table.GetForType(
      mojom::AdType::kPromotedContentAd,
      base::BindOnce(&EventHandler::OnGetAdEvents, base::Unretained(this), ad,
                     event_type));
}

void EventHandler::OnGetAdEvents(
    const PromotedContentAdInfo& ad,
    const mojom::PromotedContentAdEventType event_type,
    const bool success,
    const AdEventList& ad_events) {
  if (!success) {
    BLOG(1, "Promoted content ad: Failed to get ad events");
    return FailedToFireEvent(ad.placement_id, ad.creative_instance_id,
                             event_type);
  }

  if (!WasAdServed(ad, ad_events, event_type)) {
    BLOG(1,
         "Promoted content ad: Not allowed because an ad was not served "
         "for placement id "
             << ad.placement_id);
    return FailedToFireEvent(ad.placement_id, ad.creative_instance_id,
                             event_type);
  }

  if (ShouldDebounceAdEvent(ad, ad_events, event_type)) {
    BLOG(1, "Promoted content ad: Not allowed as debounced "
                << event_type << " event for placement id " << ad.placement_id);
    return FailedToFireEvent(ad.placement_id, ad.creative_instance_id,
                             event_type);
  }

  const auto ad_event = AdEventFactory::Build(event_type);
  ad_event->FireEvent(ad);

  NotifyPromotedContentAdEvent(ad, event_type);
}

void EventHandler::FailedToFireEvent(
    const std::string& placement_id,
    const std::string& creative_instance_id,
    const mojom::PromotedContentAdEventType event_type) const {
  DCHECK(mojom::IsKnownEnumValue(event_type));

  BLOG(1, "Failed to fire promoted content ad "
              << event_type << " event for placement id " << placement_id
              << " and creative instance id " << creative_instance_id);

  NotifyPromotedContentAdEventFailed(placement_id, creative_instance_id,
                                     event_type);
}

void EventHandler::NotifyPromotedContentAdEvent(
    const PromotedContentAdInfo& ad,
    const mojom::PromotedContentAdEventType event_type) const {
  DCHECK(mojom::IsKnownEnumValue(event_type));

  switch (event_type) {
    case mojom::PromotedContentAdEventType::kServed: {
      NotifyPromotedContentAdServed(ad);
      break;
    }

    case mojom::PromotedContentAdEventType::kViewed: {
      NotifyPromotedContentAdViewed(ad);
      break;
    }

    case mojom::PromotedContentAdEventType::kClicked: {
      NotifyPromotedContentAdClicked(ad);
      break;
    }
  }
}

void EventHandler::NotifyPromotedContentAdServed(
    const PromotedContentAdInfo& ad) const {
  for (EventHandlerObserver& observer : observers_) {
    observer.OnPromotedContentAdServed(ad);
  }
}

void EventHandler::NotifyPromotedContentAdViewed(
    const PromotedContentAdInfo& ad) const {
  for (EventHandlerObserver& observer : observers_) {
    observer.OnPromotedContentAdViewed(ad);
  }
}

void EventHandler::NotifyPromotedContentAdClicked(
    const PromotedContentAdInfo& ad) const {
  for (EventHandlerObserver& observer : observers_) {
    observer.OnPromotedContentAdClicked(ad);
  }
}

void EventHandler::NotifyPromotedContentAdEventFailed(
    const std::string& placement_id,
    const std::string& creative_instance_id,
    const mojom::PromotedContentAdEventType event_type) const {
  DCHECK(mojom::IsKnownEnumValue(event_type));

  for (EventHandlerObserver& observer : observers_) {
    observer.OnPromotedContentAdEventFailed(placement_id, creative_instance_id,
                                            event_type);
  }
}

}  // namespace ads::promoted_content_ads
