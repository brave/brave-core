/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/ads/ad_events/inline_content_ads/inline_content_ad_event_handler.h"

#include "base/check.h"
#include "bat/ads/inline_content_ad_info.h"
#include "bat/ads/internal/ads/ad_events/ad_event_info.h"
#include "bat/ads/internal/ads/ad_events/ad_event_util.h"
#include "bat/ads/internal/ads/ad_events/ad_events_database_table.h"
#include "bat/ads/internal/ads/ad_events/inline_content_ads/inline_content_ad_event_factory.h"
#include "bat/ads/internal/base/logging_util.h"
#include "bat/ads/internal/creatives/inline_content_ads/creative_inline_content_ad_info.h"
#include "bat/ads/internal/creatives/inline_content_ads/creative_inline_content_ads_database_table.h"
#include "bat/ads/internal/creatives/inline_content_ads/inline_content_ad_builder.h"

namespace ads::inline_content_ads {

namespace {

bool ShouldDebounceAdEvent(const AdInfo& ad,
                           const AdEventList& ad_events,
                           const mojom::InlineContentAdEventType& event_type) {
  DCHECK(mojom::IsKnownEnumValue(event_type));

  if (event_type == mojom::InlineContentAdEventType::kViewed &&
      HasFiredAdEvent(ad, ad_events, ConfirmationType::kViewed)) {
    return true;
  }

  if (event_type == mojom::InlineContentAdEventType::kClicked &&
      HasFiredAdEvent(ad, ad_events, ConfirmationType::kClicked)) {
    return true;
  }

  return false;
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

void EventHandler::FireEvent(const std::string& placement_id,
                             const std::string& creative_instance_id,
                             const mojom::InlineContentAdEventType event_type) {
  DCHECK(mojom::IsKnownEnumValue(event_type));

  if (placement_id.empty()) {
    BLOG(1,
         "Failed to fire inline content ad event due to an invalid placement "
         "id");
    FailedToFireEvent(placement_id, creative_instance_id, event_type);
    return;
  }

  if (creative_instance_id.empty()) {
    BLOG(1,
         "Failed to fire inline content ad event due to an invalid creative "
         "instance id");
    FailedToFireEvent(placement_id, creative_instance_id, event_type);
    return;
  }

  database::table::CreativeInlineContentAds database_table;
  database_table.GetForCreativeInstanceId(
      creative_instance_id,
      [=](const bool success, const std::string& creative_instance_id,
          const CreativeInlineContentAdInfo& creative_ad) {
        if (!success) {
          BLOG(1,
               "Failed to fire inline content ad event due to missing creative "
               "instance id "
                   << creative_instance_id);
          FailedToFireEvent(placement_id, creative_instance_id, event_type);
          return;
        }

        const InlineContentAdInfo ad =
            BuildInlineContentAd(creative_ad, placement_id);

        FireEvent(ad, placement_id, creative_instance_id, event_type);
      });
}

///////////////////////////////////////////////////////////////////////////////

void EventHandler::FireEvent(const InlineContentAdInfo& ad,
                             const std::string& placement_id,
                             const std::string& creative_instance_id,
                             const mojom::InlineContentAdEventType event_type) {
  DCHECK(mojom::IsKnownEnumValue(event_type));

  database::table::AdEvents database_table;
  database_table.GetForType(
      mojom::AdType::kInlineContentAd,
      [=](const bool success, const AdEventList& ad_events) {
        if (!success) {
          BLOG(1, "Inline content ad: Failed to get ad events");
          FailedToFireEvent(placement_id, creative_instance_id, event_type);
          return;
        }

        if (ShouldDebounceAdEvent(ad, ad_events, event_type)) {
          BLOG(1, "Inline content ad: Not allowed as already fired "
                      << event_type << " event for this placement id "
                      << placement_id);
          FailedToFireEvent(placement_id, creative_instance_id, event_type);
          return;
        }

        const auto ad_event = AdEventFactory::Build(event_type);
        ad_event->FireEvent(ad);

        NotifyInlineContentAdEvent(ad, event_type);
      });
}

void EventHandler::FailedToFireEvent(
    const std::string& placement_id,
    const std::string& creative_instance_id,
    const mojom::InlineContentAdEventType event_type) const {
  DCHECK(mojom::IsKnownEnumValue(event_type));

  BLOG(1, "Failed to fire inline content ad "
              << event_type << " event for placement id " << placement_id
              << " and creative instance id " << creative_instance_id);

  NotifyInlineContentAdEventFailed(placement_id, creative_instance_id,
                                   event_type);
}

void EventHandler::NotifyInlineContentAdEvent(
    const InlineContentAdInfo& ad,
    const mojom::InlineContentAdEventType event_type) const {
  DCHECK(mojom::IsKnownEnumValue(event_type));

  switch (event_type) {
    case mojom::InlineContentAdEventType::kServed: {
      NotifyInlineContentAdServed(ad);
      break;
    }

    case mojom::InlineContentAdEventType::kViewed: {
      NotifyInlineContentAdViewed(ad);
      break;
    }

    case mojom::InlineContentAdEventType::kClicked: {
      NotifyInlineContentAdClicked(ad);
      break;
    }
  }
}

void EventHandler::NotifyInlineContentAdServed(
    const InlineContentAdInfo& ad) const {
  for (EventHandlerObserver& observer : observers_) {
    observer.OnInlineContentAdServed(ad);
  }
}

void EventHandler::NotifyInlineContentAdViewed(
    const InlineContentAdInfo& ad) const {
  for (EventHandlerObserver& observer : observers_) {
    observer.OnInlineContentAdViewed(ad);
  }
}

void EventHandler::NotifyInlineContentAdClicked(
    const InlineContentAdInfo& ad) const {
  for (EventHandlerObserver& observer : observers_) {
    observer.OnInlineContentAdClicked(ad);
  }
}

void EventHandler::NotifyInlineContentAdEventFailed(
    const std::string& placement_id,
    const std::string& creative_instance_id,
    const mojom::InlineContentAdEventType event_type) const {
  DCHECK(mojom::IsKnownEnumValue(event_type));

  for (EventHandlerObserver& observer : observers_) {
    observer.OnInlineContentAdEventFailed(placement_id, creative_instance_id,
                                          event_type);
  }
}

}  // namespace ads::inline_content_ads
