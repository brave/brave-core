/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/ads/ad_events/new_tab_page_ads/new_tab_page_ad_event_handler.h"

#include "base/check.h"
#include "base/functional/bind.h"
#include "bat/ads/internal/account/account_util.h"
#include "bat/ads/internal/ads/ad_events/ad_event_util.h"
#include "bat/ads/internal/ads/ad_events/ad_events_database_table.h"
#include "bat/ads/internal/ads/ad_events/new_tab_page_ads/new_tab_page_ad_event_factory.h"
#include "bat/ads/internal/ads/serving/permission_rules/new_tab_page_ads/new_tab_page_ad_permission_rules.h"
#include "bat/ads/internal/common/logging_util.h"
#include "bat/ads/internal/creatives/new_tab_page_ads/creative_new_tab_page_ad_info.h"
#include "bat/ads/internal/creatives/new_tab_page_ads/creative_new_tab_page_ads_database_table.h"
#include "bat/ads/internal/creatives/new_tab_page_ads/new_tab_page_ad_builder.h"
#include "bat/ads/new_tab_page_ad_info.h"

namespace ads::new_tab_page_ads {

namespace {

bool ShouldDebounceViewedAdEvent(
    const AdInfo& ad,
    const AdEventList& ad_events,
    const mojom::NewTabPageAdEventType& event_type) {
  DCHECK(mojom::IsKnownEnumValue(event_type));

  return event_type == mojom::NewTabPageAdEventType::kViewed &&
         HasFiredAdEvent(ad, ad_events, ConfirmationType::kViewed);
}

bool ShouldDebounceClickedAdEvent(
    const AdInfo& ad,
    const AdEventList& ad_events,
    const mojom::NewTabPageAdEventType& event_type) {
  DCHECK(mojom::IsKnownEnumValue(event_type));

  return event_type == mojom::NewTabPageAdEventType::kClicked &&
         HasFiredAdEvent(ad, ad_events, ConfirmationType::kClicked);
}

bool WasAdServed(const AdInfo& ad,
                 const AdEventList& ad_events,
                 const mojom::NewTabPageAdEventType& event_type) {
  DCHECK(mojom::IsKnownEnumValue(event_type));

  return event_type == mojom::NewTabPageAdEventType::kServed ||
         HasFiredAdEvent(ad, ad_events, ConfirmationType::kServed);
}

bool IsAdPlaced(const AdInfo& ad,
                const AdEventList& ad_events,
                const mojom::NewTabPageAdEventType& event_type) {
  DCHECK(mojom::IsKnownEnumValue(event_type));

  return event_type == mojom::NewTabPageAdEventType::kServed ||
         event_type == mojom::NewTabPageAdEventType::kViewed ||
         (HasFiredAdEvent(ad, ad_events, ConfirmationType::kServed) &&
          HasFiredAdEvent(ad, ad_events, ConfirmationType::kViewed));
}

bool ShouldDebounceAdEvent(const AdInfo& ad,
                           const AdEventList& ad_events,
                           const mojom::NewTabPageAdEventType& event_type) {
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

void EventHandler::FireEvent(const std::string& placement_id,
                             const std::string& creative_instance_id,
                             const mojom::NewTabPageAdEventType event_type) {
  DCHECK(mojom::IsKnownEnumValue(event_type));

  if (placement_id.empty()) {
    BLOG(1,
         "Failed to fire new tab page ad event due to an invalid placement id");
    return FailedToFireEvent(placement_id, creative_instance_id, event_type);
  }

  if (creative_instance_id.empty()) {
    BLOG(1,
         "Failed to fire new tab page ad event due to an invalid creative "
         "instance id");
    return FailedToFireEvent(placement_id, creative_instance_id, event_type);
  }

  // We need to apply permission rules for new tab page ad served events if
  // Brave Ads is disabled.
  if (!ShouldRewardUser() &&
      event_type == mojom::NewTabPageAdEventType::kServed &&
      !PermissionRules::HasPermission()) {
    BLOG(1, "New tab page ad: Not allowed due to permission rules");
    return FailedToFireEvent(placement_id, creative_instance_id, event_type);
  }

  const database::table::CreativeNewTabPageAds database_table;
  database_table.GetForCreativeInstanceId(
      creative_instance_id,
      base::BindOnce(&EventHandler::OnGetForCreativeInstanceId,
                     base::Unretained(this), placement_id, event_type));
}

///////////////////////////////////////////////////////////////////////////////

void EventHandler::OnGetForCreativeInstanceId(
    const std::string& placement_id,
    const mojom::NewTabPageAdEventType event_type,
    const bool success,
    const std::string& creative_instance_id,
    const CreativeNewTabPageAdInfo& creative_ad) {
  if (!success) {
    BLOG(1,
         "Failed to fire new tab page ad event due to missing creative "
         "instance id "
             << creative_instance_id);
    return FailedToFireEvent(placement_id, creative_instance_id, event_type);
  }

  const NewTabPageAdInfo ad = BuildNewTabPageAd(creative_ad, placement_id);
  FireEvent(ad, event_type);
}

void EventHandler::FireEvent(const NewTabPageAdInfo& ad,
                             const mojom::NewTabPageAdEventType event_type) {
  DCHECK(mojom::IsKnownEnumValue(event_type));

  const database::table::AdEvents database_table;
  database_table.GetForType(
      mojom::AdType::kNewTabPageAd,
      base::BindOnce(&EventHandler::OnGetAdEvents, base::Unretained(this), ad,
                     event_type));
}

void EventHandler::OnGetAdEvents(const NewTabPageAdInfo& ad,
                                 const mojom::NewTabPageAdEventType event_type,
                                 const bool success,
                                 const AdEventList& ad_events) {
  if (!success) {
    BLOG(1, "New tab page ad: Failed to get ad events");
    return FailedToFireEvent(ad.placement_id, ad.creative_instance_id,
                             event_type);
  }

  if (!WasAdServed(ad, ad_events, event_type)) {
    BLOG(1,
         "New tab page ad: Not allowed because an ad was not served "
         "for placement id "
             << ad.placement_id);
    return FailedToFireEvent(ad.placement_id, ad.creative_instance_id,
                             event_type);
  }

  if (ShouldDebounceAdEvent(ad, ad_events, event_type)) {
    BLOG(1, "New tab page ad: Not allowed as debounced "
                << event_type << " event for placement id " << ad.placement_id);
    return FailedToFireEvent(ad.placement_id, ad.creative_instance_id,
                             event_type);
  }

  const auto ad_event = AdEventFactory::Build(event_type);
  ad_event->FireEvent(ad);

  NotifyNewTabPageAdEvent(ad, event_type);
}

void EventHandler::FailedToFireEvent(
    const std::string& placement_id,
    const std::string& creative_instance_id,
    const mojom::NewTabPageAdEventType event_type) const {
  DCHECK(mojom::IsKnownEnumValue(event_type));

  BLOG(1, "Failed to fire new tab page ad "
              << event_type << " event for placement id " << placement_id
              << " and creative instance id " << creative_instance_id);

  NotifyNewTabPageAdEventFailed(placement_id, creative_instance_id, event_type);
}

void EventHandler::NotifyNewTabPageAdEvent(
    const NewTabPageAdInfo& ad,
    const mojom::NewTabPageAdEventType event_type) const {
  DCHECK(mojom::IsKnownEnumValue(event_type));

  switch (event_type) {
    case mojom::NewTabPageAdEventType::kServed: {
      NotifyNewTabPageAdServed(ad);
      break;
    }

    case mojom::NewTabPageAdEventType::kViewed: {
      NotifyNewTabPageAdViewed(ad);
      break;
    }

    case mojom::NewTabPageAdEventType::kClicked: {
      NotifyNewTabPageAdClicked(ad);
      break;
    }
  }
}

void EventHandler::NotifyNewTabPageAdServed(const NewTabPageAdInfo& ad) const {
  for (EventHandlerObserver& observer : observers_) {
    observer.OnNewTabPageAdServed(ad);
  }
}

void EventHandler::NotifyNewTabPageAdViewed(const NewTabPageAdInfo& ad) const {
  for (EventHandlerObserver& observer : observers_) {
    observer.OnNewTabPageAdViewed(ad);
  }
}

void EventHandler::NotifyNewTabPageAdClicked(const NewTabPageAdInfo& ad) const {
  for (EventHandlerObserver& observer : observers_) {
    observer.OnNewTabPageAdClicked(ad);
  }
}

void EventHandler::NotifyNewTabPageAdEventFailed(
    const std::string& placement_id,
    const std::string& creative_instance_id,
    const mojom::NewTabPageAdEventType event_type) const {
  DCHECK(mojom::IsKnownEnumValue(event_type));

  for (EventHandlerObserver& observer : observers_) {
    observer.OnNewTabPageAdEventFailed(placement_id, creative_instance_id,
                                       event_type);
  }
}

}  // namespace ads::new_tab_page_ads
