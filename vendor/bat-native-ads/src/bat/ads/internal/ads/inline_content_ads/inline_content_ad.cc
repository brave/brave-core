/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/ads/inline_content_ads/inline_content_ad.h"

#include "base/check.h"
#include "bat/ads/inline_content_ad_info.h"
#include "bat/ads/internal/ad_events/ad_event_info.h"
#include "bat/ads/internal/ad_events/ad_event_util.h"
#include "bat/ads/internal/ad_events/inline_content_ads/inline_content_ad_event_factory.h"
#include "bat/ads/internal/ads/inline_content_ads/inline_content_ad_builder.h"
#include "bat/ads/internal/bundle/creative_inline_content_ad_info.h"
#include "bat/ads/internal/database/tables/ad_events_database_table.h"
#include "bat/ads/internal/database/tables/creative_inline_content_ads_database_table.h"
#include "bat/ads/internal/logging.h"

namespace ads {

InlineContentAd::InlineContentAd() = default;

InlineContentAd::~InlineContentAd() = default;

void InlineContentAd::AddObserver(InlineContentAdObserver* observer) {
  DCHECK(observer);
  observers_.AddObserver(observer);
}

void InlineContentAd::RemoveObserver(InlineContentAdObserver* observer) {
  DCHECK(observer);
  observers_.RemoveObserver(observer);
}

void InlineContentAd::FireEvent(
    const std::string& uuid,
    const std::string& creative_instance_id,
    const mojom::InlineContentAdEventType event_type) {
  if (uuid.empty() || creative_instance_id.empty()) {
    BLOG(1, "Failed to fire inline content ad event due to invalid uuid "
                << uuid << " or creative instance id " << creative_instance_id);
    NotifyInlineContentAdEventFailed(uuid, creative_instance_id, event_type);
    return;
  }

  database::table::CreativeInlineContentAds database_table;
  database_table.GetForCreativeInstanceId(
      creative_instance_id,
      [=](const bool success, const std::string& creative_instance_id,
          const CreativeInlineContentAdInfo& creative_inline_content_ad) {
        if (!success) {
          BLOG(1,
               "Failed to fire inline content ad event due to missing creative "
               "instance id "
                   << creative_instance_id);
          NotifyInlineContentAdEventFailed(uuid, creative_instance_id,
                                           event_type);
          return;
        }

        const InlineContentAdInfo ad =
            BuildInlineContentAd(creative_inline_content_ad, uuid);

        FireEvent(ad, uuid, creative_instance_id, event_type);
      });
}

///////////////////////////////////////////////////////////////////////////////

void InlineContentAd::FireEvent(
    const InlineContentAdInfo& ad,
    const std::string& uuid,
    const std::string& creative_instance_id,
    const mojom::InlineContentAdEventType event_type) {
  database::table::AdEvents database_table;
  database_table.GetAll([=](const bool success, const AdEventList& ad_events) {
    if (!success) {
      BLOG(1, "Inline content ad: Failed to get ad events");
      NotifyInlineContentAdEventFailed(uuid, creative_instance_id, event_type);
      return;
    }

    if (event_type == mojom::InlineContentAdEventType::kViewed &&
        HasFiredAdViewedEvent(ad, ad_events)) {
      BLOG(1, "Inline content ad: Not allowed as already viewed uuid " << uuid);
      NotifyInlineContentAdEventFailed(uuid, creative_instance_id, event_type);
      return;
    }

    const auto ad_event = inline_content_ads::AdEventFactory::Build(event_type);
    ad_event->FireEvent(ad);

    NotifyInlineContentAdEvent(ad, event_type);
  });
}

void InlineContentAd::NotifyInlineContentAdEvent(
    const InlineContentAdInfo& ad,
    const mojom::InlineContentAdEventType event_type) const {
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

void InlineContentAd::NotifyInlineContentAdServed(
    const InlineContentAdInfo& ad) const {
  for (InlineContentAdObserver& observer : observers_) {
    observer.OnInlineContentAdServed(ad);
  }
}

void InlineContentAd::NotifyInlineContentAdViewed(
    const InlineContentAdInfo& ad) const {
  for (InlineContentAdObserver& observer : observers_) {
    observer.OnInlineContentAdViewed(ad);
  }
}

void InlineContentAd::NotifyInlineContentAdClicked(
    const InlineContentAdInfo& ad) const {
  for (InlineContentAdObserver& observer : observers_) {
    observer.OnInlineContentAdClicked(ad);
  }
}

void InlineContentAd::NotifyInlineContentAdEventFailed(
    const std::string& uuid,
    const std::string& creative_instance_id,
    const mojom::InlineContentAdEventType event_type) const {
  for (InlineContentAdObserver& observer : observers_) {
    observer.OnInlineContentAdEventFailed(uuid, creative_instance_id,
                                          event_type);
  }
}

}  // namespace ads
