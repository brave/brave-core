/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/ads/promoted_content_ads/promoted_content_ad.h"

#include "bat/ads/internal/ad_events/promoted_content_ads/promoted_content_ad_event_factory.h"
#include "bat/ads/internal/bundle/creative_promoted_content_ad_info.h"
#include "bat/ads/internal/database/tables/ad_events_database_table.h"
#include "bat/ads/internal/database/tables/creative_promoted_content_ads_database_table.h"
#include "bat/ads/internal/frequency_capping/promoted_content_ads/promoted_content_ads_frequency_capping.h"
#include "bat/ads/internal/logging.h"
#include "bat/ads/promoted_content_ad_info.h"

namespace ads {

namespace {

PromotedContentAdInfo CreatePromotedContentAd(
    const std::string& uuid,
    const CreativePromotedContentAdInfo& ad) {
  PromotedContentAdInfo promoted_content_ad;

  promoted_content_ad.type = AdType::kPromotedContentAd;
  promoted_content_ad.uuid = uuid;
  promoted_content_ad.creative_instance_id = ad.creative_instance_id;
  promoted_content_ad.creative_set_id = ad.creative_set_id;
  promoted_content_ad.campaign_id = ad.campaign_id;
  promoted_content_ad.advertiser_id = ad.advertiser_id;
  promoted_content_ad.segment = ad.segment;
  promoted_content_ad.target_url = ad.target_url;
  promoted_content_ad.title = ad.title;
  promoted_content_ad.description = ad.description;

  return promoted_content_ad;
}

}  // namespace

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
    BLOG(1, "Failed to fire promoted content ad event for uuid "
                << uuid << " and creative instance id "
                << creative_instance_id);

    NotifyPromotedContentAdEventFailed(uuid, creative_instance_id, event_type);

    return;
  }

  database::table::CreativePromotedContentAds database_table;
  database_table.GetForCreativeInstanceId(
      creative_instance_id,
      [=](const Result result, const std::string& creative_instance_id,
          const CreativePromotedContentAdInfo& creative_promoted_content_ad) {
        if (result != SUCCESS) {
          BLOG(1, "Failed to fire promoted content ad event for uuid");

          NotifyPromotedContentAdEventFailed(uuid, creative_instance_id,
                                             event_type);

          return;
        }

        const PromotedContentAdInfo ad =
            CreatePromotedContentAd(uuid, creative_promoted_content_ad);

        FireEvent(ad, uuid, creative_instance_id, event_type);
      });
}

///////////////////////////////////////////////////////////////////////////////

bool PromotedContentAd::ShouldFireEvent(const PromotedContentAdInfo& ad,
                                        const AdEventList& ad_events) {
  promoted_content_ads::FrequencyCapping frequency_capping(ad_events);

  if (!frequency_capping.IsAdAllowed()) {
    return false;
  }

  if (frequency_capping.ShouldExcludeAd(ad)) {
    return false;
  }

  return true;
}

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

    if (event_type == PromotedContentAdEventType::kViewed &&
        !ShouldFireEvent(ad, ad_events)) {
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
    const PromotedContentAdEventType event_type) {
  switch (event_type) {
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

void PromotedContentAd::NotifyPromotedContentAdViewed(
    const PromotedContentAdInfo& ad) {
  for (PromotedContentAdObserver& observer : observers_) {
    observer.OnPromotedContentAdViewed(ad);
  }
}

void PromotedContentAd::NotifyPromotedContentAdClicked(
    const PromotedContentAdInfo& ad) {
  for (PromotedContentAdObserver& observer : observers_) {
    observer.OnPromotedContentAdClicked(ad);
  }
}

void PromotedContentAd::NotifyPromotedContentAdEventFailed(
    const std::string& uuid,
    const std::string& creative_instance_id,
    const PromotedContentAdEventType event_type) {
  for (PromotedContentAdObserver& observer : observers_) {
    observer.OnPromotedContentAdEventFailed(uuid, creative_instance_id,
                                            event_type);
  }
}

}  // namespace ads
