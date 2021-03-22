/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/ads/new_tab_page_ads/new_tab_page_ad.h"

#include "bat/ads/internal/ad_events/new_tab_page_ads/new_tab_page_ad_event_factory.h"
#include "bat/ads/internal/bundle/creative_new_tab_page_ad_info.h"
#include "bat/ads/internal/database/tables/ad_events_database_table.h"
#include "bat/ads/internal/database/tables/creative_new_tab_page_ads_database_table.h"
#include "bat/ads/internal/frequency_capping/new_tab_page_ads/new_tab_page_ads_frequency_capping.h"
#include "bat/ads/internal/logging.h"
#include "bat/ads/new_tab_page_ad_info.h"

namespace ads {

namespace {

NewTabPageAdInfo CreateNewTabPageAd(const std::string& uuid,
                                    const CreativeNewTabPageAdInfo& ad) {
  NewTabPageAdInfo new_tab_page_ad;

  new_tab_page_ad.type = AdType::kNewTabPageAd;
  new_tab_page_ad.uuid = uuid;
  new_tab_page_ad.creative_instance_id = ad.creative_instance_id;
  new_tab_page_ad.creative_set_id = ad.creative_set_id;
  new_tab_page_ad.campaign_id = ad.campaign_id;
  new_tab_page_ad.advertiser_id = ad.advertiser_id;
  new_tab_page_ad.segment = ad.segment;
  new_tab_page_ad.target_url = ad.target_url;
  new_tab_page_ad.company_name = ad.company_name;
  new_tab_page_ad.alt = ad.alt;

  return new_tab_page_ad;
}

}  // namespace

NewTabPageAd::NewTabPageAd() = default;

NewTabPageAd::~NewTabPageAd() = default;

void NewTabPageAd::AddObserver(NewTabPageAdObserver* observer) {
  DCHECK(observer);
  observers_.AddObserver(observer);
}

void NewTabPageAd::RemoveObserver(NewTabPageAdObserver* observer) {
  DCHECK(observer);
  observers_.RemoveObserver(observer);
}

void NewTabPageAd::FireEvent(const std::string& uuid,
                             const std::string& creative_instance_id,
                             const NewTabPageAdEventType event_type) {
  if (uuid.empty() || creative_instance_id.empty()) {
    BLOG(1, "Failed to fire new tab page ad event for uuid "
                << uuid << " and creative instance id "
                << creative_instance_id);

    NotifyNewTabPageAdEventFailed(uuid, creative_instance_id, event_type);

    return;
  }

  database::table::CreativeNewTabPageAds database_table;
  database_table.GetForCreativeInstanceId(
      creative_instance_id,
      [=](const Result result, const std::string& creative_instance_id,
          const CreativeNewTabPageAdInfo& creative_new_tab_page_ad) {
        if (result != SUCCESS) {
          BLOG(1, "Failed to fire new tab page ad event for uuid");

          NotifyNewTabPageAdEventFailed(uuid, creative_instance_id, event_type);

          return;
        }

        const NewTabPageAdInfo ad =
            CreateNewTabPageAd(uuid, creative_new_tab_page_ad);

        FireEvent(ad, uuid, creative_instance_id, event_type);
      });
}

///////////////////////////////////////////////////////////////////////////////

bool NewTabPageAd::ShouldFireEvent(const NewTabPageAdInfo& ad,
                                   const AdEventList& ad_events) {
  new_tab_page_ads::FrequencyCapping frequency_capping(ad_events);

  if (!frequency_capping.IsAdAllowed()) {
    return false;
  }

  if (frequency_capping.ShouldExcludeAd(ad)) {
    return false;
  }

  return true;
}

void NewTabPageAd::FireEvent(const NewTabPageAdInfo& ad,
                             const std::string& uuid,
                             const std::string& creative_instance_id,
                             const NewTabPageAdEventType event_type) {
  database::table::AdEvents database_table;
  database_table.GetAll([=](const Result result, const AdEventList& ad_events) {
    if (result != Result::SUCCESS) {
      BLOG(1, "New tab page ad: Failed to get ad events");

      NotifyNewTabPageAdEventFailed(uuid, creative_instance_id, event_type);

      return;
    }

    if (event_type == NewTabPageAdEventType::kViewed &&
        !ShouldFireEvent(ad, ad_events)) {
      BLOG(1, "New tab page ad: Not allowed");

      NotifyNewTabPageAdEventFailed(uuid, creative_instance_id, event_type);

      return;
    }

    const auto ad_event = new_tab_page_ads::AdEventFactory::Build(event_type);
    ad_event->FireEvent(ad);

    NotifyNewTabPageAdEvent(ad, event_type);
  });
}

void NewTabPageAd::NotifyNewTabPageAdEvent(
    const NewTabPageAdInfo& ad,
    const NewTabPageAdEventType event_type) {
  switch (event_type) {
    case NewTabPageAdEventType::kViewed: {
      NotifyNewTabPageAdViewed(ad);
      break;
    }

    case NewTabPageAdEventType::kClicked: {
      NotifyNewTabPageAdClicked(ad);
      break;
    }
  }
}

void NewTabPageAd::NotifyNewTabPageAdViewed(const NewTabPageAdInfo& ad) {
  for (NewTabPageAdObserver& observer : observers_) {
    observer.OnNewTabPageAdViewed(ad);
  }
}

void NewTabPageAd::NotifyNewTabPageAdClicked(const NewTabPageAdInfo& ad) {
  for (NewTabPageAdObserver& observer : observers_) {
    observer.OnNewTabPageAdClicked(ad);
  }
}

void NewTabPageAd::NotifyNewTabPageAdEventFailed(
    const std::string& uuid,
    const std::string& creative_instance_id,
    const NewTabPageAdEventType event_type) {
  for (NewTabPageAdObserver& observer : observers_) {
    observer.OnNewTabPageAdEventFailed(uuid, creative_instance_id, event_type);
  }
}

}  // namespace ads
