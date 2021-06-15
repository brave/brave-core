/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/ads/new_tab_page_ads/new_tab_page_ad.h"

#include "bat/ads/internal/ad_events/ad_event_util.h"
#include "bat/ads/internal/ad_events/new_tab_page_ads/new_tab_page_ad_event_factory.h"
#include "bat/ads/internal/ads/new_tab_page_ads/new_tab_page_ad_builder.h"
#include "bat/ads/internal/ads/new_tab_page_ads/new_tab_page_ad_permission_rules.h"
#include "bat/ads/internal/bundle/creative_new_tab_page_ad_info.h"
#include "bat/ads/internal/database/tables/ad_events_database_table.h"
#include "bat/ads/internal/database/tables/creative_new_tab_page_ads_database_table.h"
#include "bat/ads/internal/logging.h"
#include "bat/ads/new_tab_page_ad_info.h"

namespace ads {

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
    BLOG(1, "Failed to fire new tab page ad event due to invalid uuid "
                << uuid << " or creative instance id " << creative_instance_id);
    NotifyNewTabPageAdEventFailed(uuid, creative_instance_id, event_type);
    return;
  }

  new_tab_page_ads::frequency_capping::PermissionRules permission_rules;
  if (event_type == NewTabPageAdEventType::kViewed &&
      !permission_rules.HasPermission()) {
    BLOG(1, "New tab page ad: Not allowed due to permission rules");
    NotifyNewTabPageAdEventFailed(uuid, creative_instance_id, event_type);
    return;
  }

  database::table::CreativeNewTabPageAds database_table;
  database_table.GetForCreativeInstanceId(
      creative_instance_id,
      [=](const Result result, const std::string& creative_instance_id,
          const CreativeNewTabPageAdInfo& creative_new_tab_page_ad) {
        if (result != SUCCESS) {
          BLOG(1,
               "Failed to fire new tab page ad event due to missing creative "
               "instance id "
                   << creative_instance_id);
          NotifyNewTabPageAdEventFailed(uuid, creative_instance_id, event_type);
          return;
        }

        const NewTabPageAdInfo ad =
            BuildNewTabPageAd(creative_new_tab_page_ad, uuid);

        FireEvent(ad, uuid, creative_instance_id, event_type);
      });
}

///////////////////////////////////////////////////////////////////////////////

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

    if (HasFiredAdViewedEvent(ad, ad_events)) {
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
    const NewTabPageAdEventType event_type) const {
  switch (event_type) {
    case NewTabPageAdEventType::kServed: {
      NotifyNewTabPageAdServed(ad);
      break;
    }

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

void NewTabPageAd::NotifyNewTabPageAdServed(const NewTabPageAdInfo& ad) const {
  for (NewTabPageAdObserver& observer : observers_) {
    observer.OnNewTabPageAdServed(ad);
  }
}

void NewTabPageAd::NotifyNewTabPageAdViewed(const NewTabPageAdInfo& ad) const {
  for (NewTabPageAdObserver& observer : observers_) {
    observer.OnNewTabPageAdViewed(ad);
  }
}

void NewTabPageAd::NotifyNewTabPageAdClicked(const NewTabPageAdInfo& ad) const {
  for (NewTabPageAdObserver& observer : observers_) {
    observer.OnNewTabPageAdClicked(ad);
  }
}

void NewTabPageAd::NotifyNewTabPageAdEventFailed(
    const std::string& uuid,
    const std::string& creative_instance_id,
    const NewTabPageAdEventType event_type) const {
  for (NewTabPageAdObserver& observer : observers_) {
    observer.OnNewTabPageAdEventFailed(uuid, creative_instance_id, event_type);
  }
}

}  // namespace ads
