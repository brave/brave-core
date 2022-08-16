/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/ads/ad_events/promoted_content_ads/promoted_content_ad_event_handler.h"

#include "base/check.h"
#include "bat/ads/internal/ads/ad_events/ad_event_info.h"
#include "bat/ads/internal/ads/ad_events/ad_event_util.h"
#include "bat/ads/internal/ads/ad_events/ad_events_database_table.h"
#include "bat/ads/internal/ads/ad_events/promoted_content_ads/promoted_content_ad_event_factory.h"
#include "bat/ads/internal/ads/serving/permission_rules/promoted_content_ads/promoted_content_ad_permission_rules.h"
#include "bat/ads/internal/base/logging_util.h"
#include "bat/ads/internal/creatives/promoted_content_ads/creative_promoted_content_ad_info.h"
#include "bat/ads/internal/creatives/promoted_content_ads/creative_promoted_content_ads_database_table.h"
#include "bat/ads/internal/creatives/promoted_content_ads/promoted_content_ad_builder.h"
#include "bat/ads/promoted_content_ad_info.h"

namespace ads {
namespace promoted_content_ads {

namespace {

bool ShouldDebounceAdEvent(
    const AdInfo& ad,
    const AdEventList& ad_events,
    const mojom::PromotedContentAdEventType& event_type) {
  DCHECK(mojom::IsKnownEnumValue(event_type));

  if (event_type == mojom::PromotedContentAdEventType::kViewed &&
      HasFiredAdEvent(ad, ad_events, ConfirmationType::kViewed)) {
    return true;
  } else if (event_type == mojom::PromotedContentAdEventType::kClicked &&
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

void EventHandler::FireEvent(
    const std::string& placement_id,
    const std::string& creative_instance_id,
    const mojom::PromotedContentAdEventType event_type) {
  DCHECK(mojom::IsKnownEnumValue(event_type));

  if (placement_id.empty()) {
    BLOG(1,
         "Failed to fire promoted content ad event due to an invalid placement "
         "id");
    FailedToFireEvent(placement_id, creative_instance_id, event_type);
    return;
  }

  if (creative_instance_id.empty()) {
    BLOG(1,
         "Failed to fire promoted content ad event due to an invalid creative "
         "instance id");
    FailedToFireEvent(placement_id, creative_instance_id, event_type);
    return;
  }

  PermissionRules permission_rules;
  if (!permission_rules.HasPermission()) {
    BLOG(1, "Promoted content ad: Not allowed due to permission rules");
    FailedToFireEvent(placement_id, creative_instance_id, event_type);
    return;
  }

  database::table::CreativePromotedContentAds database_table;
  database_table.GetForCreativeInstanceId(
      creative_instance_id,
      [=](const bool success, const std::string& creative_instance_id,
          const CreativePromotedContentAdInfo& creative_ad) {
        if (!success) {
          BLOG(1,
               "Failed to fire promoted content ad event due to missing "
               "creative instance id "
                   << creative_instance_id);
          FailedToFireEvent(placement_id, creative_instance_id, event_type);
          return;
        }

        const PromotedContentAdInfo ad =
            BuildPromotedContentAd(creative_ad, placement_id);

        FireEvent(ad, placement_id, creative_instance_id, event_type);
      });
}

///////////////////////////////////////////////////////////////////////////////

void EventHandler::FireEvent(
    const PromotedContentAdInfo& ad,
    const std::string& placement_id,
    const std::string& creative_instance_id,
    const mojom::PromotedContentAdEventType event_type) {
  DCHECK(mojom::IsKnownEnumValue(event_type));

  database::table::AdEvents database_table;
  database_table.GetForType(
      mojom::AdType::kPromotedContentAd,
      [=](const bool success, const AdEventList& ad_events) {
        if (!success) {
          BLOG(1, "Promoted content ad: Failed to get ad events");
          FailedToFireEvent(placement_id, creative_instance_id, event_type);
          return;
        }

        if (ShouldDebounceAdEvent(ad, ad_events, event_type)) {
          BLOG(1, "Promoted content ad: Not allowed as already fired "
                      << event_type << " event for this placement id "
                      << placement_id);
          FailedToFireEvent(placement_id, creative_instance_id, event_type);
          return;
        }

        if (event_type == mojom::PromotedContentAdEventType::kViewed) {
          // We must fire an ad served event due to promoted content ads not
          // being delivered by the library
          FireEvent(placement_id, creative_instance_id,
                    mojom::PromotedContentAdEventType::kServed);
        }

        const auto ad_event = AdEventFactory::Build(event_type);
        ad_event->FireEvent(ad);

        NotifyPromotedContentAdEvent(ad, event_type);
      });
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

}  // namespace promoted_content_ads
}  // namespace ads
