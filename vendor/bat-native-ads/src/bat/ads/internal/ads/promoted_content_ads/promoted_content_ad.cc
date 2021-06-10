/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/ads/promoted_content_ads/promoted_content_ad.h"

#include "bat/ads/internal/ad_events/ad_event_util.h"
#include "bat/ads/internal/ad_events/promoted_content_ads/promoted_content_ad_event_factory.h"
#include "bat/ads/internal/ads/promoted_content_ads/promoted_content_ad_builder.h"
#include "bat/ads/internal/ads/promoted_content_ads/promoted_content_ad_permission_rules.h"
#include "bat/ads/internal/bundle/creative_promoted_content_ad_info.h"
#include "bat/ads/internal/database/tables/ad_events_database_table.h"
#include "bat/ads/internal/database/tables/creative_promoted_content_ads_database_table.h"
#include "bat/ads/internal/logging.h"
#include "bat/ads/promoted_content_ad_info.h"

namespace ads {

PromotedContentAd::PromotedContentAd() = default;

PromotedContentAd::~PromotedContentAd() = default;

void PromotedContentAd::AddObserver(PromotedContentAdObserver* observer) {
  DCHECK(observer);
  observers_.AddObserver(observer);
}

void PromotedContentAd::RemoveObserver(PromotedContentAdObserver* observer) {
  DCHECK(observer);
  observers_.RemoveObserver(observer);
}

void PromotedContentAd::FireEvent(const std::string& uuid,
                                  const std::string& creative_instance_id,
                                  const PromotedContentAdEventType event_type) {
  if (uuid.empty() || creative_instance_id.empty()) {
    BLOG(1, "Failed to fire promoted content ad event due to invalid uuid "
                << uuid << " or creative instance id " << creative_instance_id);
    NotifyPromotedContentAdEventFailed(uuid, creative_instance_id, event_type);
    return;
  }

  promoted_content_ads::frequency_capping::PermissionRules permission_rules;
  if (!permission_rules.HasPermission()) {
    BLOG(1, "Promoted content ad: Not allowed due to permission rules");
    NotifyPromotedContentAdEventFailed(uuid, creative_instance_id, event_type);
    return;
  }

  database::table::CreativePromotedContentAds database_table;
  database_table.GetForCreativeInstanceId(
      creative_instance_id,
      [=](const Result result, const std::string& creative_instance_id,
          const CreativePromotedContentAdInfo& creative_promoted_content_ad) {
        if (result != SUCCESS) {
          BLOG(1,
               "Failed to fire promoted content ad event due to missing "
               "creative instance id "
                   << creative_instance_id);
          NotifyPromotedContentAdEventFailed(uuid, creative_instance_id,
                                             event_type);
          return;
        }

        const PromotedContentAdInfo ad =
            BuildPromotedContentAd(creative_promoted_content_ad, uuid);

        FireEvent(ad, uuid, creative_instance_id, event_type);
      });
}

///////////////////////////////////////////////////////////////////////////////

void PromotedContentAd::FireEvent(const PromotedContentAdInfo& ad,
                                  const std::string& uuid,
                                  const std::string& creative_instance_id,
                                  const PromotedContentAdEventType event_type) {
  database::table::AdEvents database_table;
  database_table.GetAll([=](const Result result, const AdEventList& ad_events) {
    if (result != Result::SUCCESS) {
      BLOG(1, "Promoted content ad: Failed to get ad events");
      NotifyPromotedContentAdEventFailed(uuid, creative_instance_id,
                                         event_type);
      return;
    }

    if (HasFiredAdViewedEvent(ad, ad_events)) {
      BLOG(1, "Promoted content ad: Not allowed");
      NotifyPromotedContentAdEventFailed(uuid, creative_instance_id,
                                         event_type);
      return;
    }

    const auto ad_event =
        promoted_content_ads::AdEventFactory::Build(event_type);
    ad_event->FireEvent(ad);

    NotifyPromotedContentAdEvent(ad, event_type);
  });
}

void PromotedContentAd::NotifyPromotedContentAdEvent(
    const PromotedContentAdInfo& ad,
    const PromotedContentAdEventType event_type) const {
  switch (event_type) {
    case PromotedContentAdEventType::kServed: {
      NotifyPromotedContentAdServed(ad);
      break;
    }

    case PromotedContentAdEventType::kViewed: {
      NotifyPromotedContentAdViewed(ad);
      break;
    }

    case PromotedContentAdEventType::kClicked: {
      NotifyPromotedContentAdClicked(ad);
      break;
    }
  }
}

void PromotedContentAd::NotifyPromotedContentAdServed(
    const PromotedContentAdInfo& ad) const {
  for (PromotedContentAdObserver& observer : observers_) {
    observer.OnPromotedContentAdServed(ad);
  }
}

void PromotedContentAd::NotifyPromotedContentAdViewed(
    const PromotedContentAdInfo& ad) const {
  for (PromotedContentAdObserver& observer : observers_) {
    observer.OnPromotedContentAdViewed(ad);
  }
}

void PromotedContentAd::NotifyPromotedContentAdClicked(
    const PromotedContentAdInfo& ad) const {
  for (PromotedContentAdObserver& observer : observers_) {
    observer.OnPromotedContentAdClicked(ad);
  }
}

void PromotedContentAd::NotifyPromotedContentAdEventFailed(
    const std::string& uuid,
    const std::string& creative_instance_id,
    const PromotedContentAdEventType event_type) const {
  for (PromotedContentAdObserver& observer : observers_) {
    observer.OnPromotedContentAdEventFailed(uuid, creative_instance_id,
                                            event_type);
  }
}

}  // namespace ads
