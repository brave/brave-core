/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/ads/new_tab_page_ads/new_tab_page_ad.h"

#include "bat/ads/internal/ad_events/new_tab_page_ads/new_tab_page_ad_event_factory.h"
#include "bat/ads/internal/bundle/creative_new_tab_page_ad_info.h"
#include "bat/ads/internal/database/tables/creative_new_tab_page_ads_database_table.h"
#include "bat/ads/internal/logging.h"
#include "bat/ads/new_tab_page_ad_info.h"

namespace ads {

namespace {

NewTabPageAdInfo CreateNewTabPageAd(
    const std::string& uuid,
    const CreativeNewTabPageAdInfo& ad) {
  NewTabPageAdInfo new_tab_page_ad;

  new_tab_page_ad.type = AdType::kNewTabPageAd;
  new_tab_page_ad.uuid = uuid;
  new_tab_page_ad.creative_instance_id = ad.creative_instance_id;
  new_tab_page_ad.creative_set_id = ad.creative_set_id;
  new_tab_page_ad.campaign_id = ad.campaign_id;
  new_tab_page_ad.category = ad.category;
  new_tab_page_ad.target_url = ad.target_url;
  new_tab_page_ad.company_name = ad.company_name;
  new_tab_page_ad.alt = ad.alt;

  return new_tab_page_ad;
}

}  // namespace

NewTabPageAd::NewTabPageAd() = default;

NewTabPageAd::~NewTabPageAd() = default;

void NewTabPageAd::AddObserver(
    NewTabPageAdObserver* observer) {
  observers_.AddObserver(observer);
}

void NewTabPageAd::RemoveObserver(
    NewTabPageAdObserver* observer) {
  observers_.RemoveObserver(observer);
}

void NewTabPageAd::FireEvent(
    const std::string& wallpaper_id,
    const std::string& creative_instance_id,
    const NewTabPageAdEventType event_type) {
  if (wallpaper_id.empty() || creative_instance_id.empty()) {
    BLOG(1, "Failed to trigger new tab page ad event for wallpaper id "
        << wallpaper_id << " and creative instance id "
            << creative_instance_id);

    return;
  }

  database::table::CreativeNewTabPageAds database_table;
  database_table.GetForCreativeInstanceId(creative_instance_id, [=](
      const Result result,
      const std::string& creative_instance_id,
      const CreativeNewTabPageAdInfo& creative_new_tab_page_ad) {
    if (result != SUCCESS) {
      BLOG(1, "Failed to trigger new tab page ad event for wallpaper id");

      NotifyNewTabPageAdEventFailed(wallpaper_id,
          creative_instance_id, event_type);

      return;
    }

    const NewTabPageAdInfo ad =
        CreateNewTabPageAd(wallpaper_id, creative_new_tab_page_ad);

    const auto ad_event = new_tab_page_ads::AdEventFactory::Build(event_type);
    ad_event->FireEvent(ad);

    NotifyNewTabPageAdEvent(ad, event_type);
  });
}

///////////////////////////////////////////////////////////////////////////////

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

void NewTabPageAd::NotifyNewTabPageAdViewed(
    const NewTabPageAdInfo& ad) {
  for (NewTabPageAdObserver& observer : observers_) {
    observer.OnNewTabPageAdViewed(ad);
  }
}

void NewTabPageAd::NotifyNewTabPageAdClicked(
    const NewTabPageAdInfo& ad) {
  for (NewTabPageAdObserver& observer : observers_) {
    observer.OnNewTabPageAdClicked(ad);
  }
}

void NewTabPageAd::NotifyNewTabPageAdEventFailed(
    const std::string& wallpaper_id,
    const std::string& creative_instance_id,
    const NewTabPageAdEventType event_type) {
  for (NewTabPageAdObserver& observer : observers_) {
    observer.OnNewTabPageAdEventFailed(wallpaper_id,
        creative_instance_id, event_type);
  }
}

}  // namespace ads
