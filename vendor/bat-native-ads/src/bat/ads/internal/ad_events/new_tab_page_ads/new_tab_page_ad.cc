/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/ad_events/new_tab_page_ads/new_tab_page_ad.h"

#include "base/check.h"
#include "bat/ads/internal/ad_events/ad_event_info.h"
#include "bat/ads/internal/ad_events/ad_event_util.h"
#include "bat/ads/internal/ad_events/ad_events_database_table.h"
#include "bat/ads/internal/ad_events/new_tab_page_ads/new_tab_page_ad_event_factory.h"
#include "bat/ads/internal/base/logging_util.h"
#include "bat/ads/internal/creatives/new_tab_page_ads/creative_new_tab_page_ad_info.h"
#include "bat/ads/internal/creatives/new_tab_page_ads/creative_new_tab_page_ads_database_table.h"
#include "bat/ads/internal/creatives/new_tab_page_ads/new_tab_page_ad_builder.h"
#include "bat/ads/internal/serving/permission_rules/new_tab_page_ads/new_tab_page_ad_permission_rules.h"
#include "bat/ads/new_tab_page_ad_info.h"

namespace ads {

namespace {

bool ShouldDebounceAdEvent(const AdInfo& ad,
                           const AdEventList& ad_events,
                           const mojom::NewTabPageAdEventType& event_type) {
  if (event_type == mojom::NewTabPageAdEventType::kViewed &&
      HasFiredAdEvent(ad, ad_events, ConfirmationType::kViewed)) {
    return true;
  } else if (event_type == mojom::NewTabPageAdEventType::kClicked &&
             HasFiredAdEvent(ad, ad_events, ConfirmationType::kClicked)) {
    return true;
  }

  return false;
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

void NewTabPageAd::FireEvent(const std::string& placement_id,
                             const std::string& creative_instance_id,
                             const mojom::NewTabPageAdEventType event_type) {
  if (placement_id.empty()) {
    BLOG(1,
         "Failed to fire new tab page ad event due to an invalid placement id");
    NotifyNewTabPageAdEventFailed(placement_id, creative_instance_id,
                                  event_type);
    return;
  }

  if (creative_instance_id.empty()) {
    BLOG(1,
         "Failed to fire new tab page ad event due to an invalid creative "
         "instance id");
    NotifyNewTabPageAdEventFailed(placement_id, creative_instance_id,
                                  event_type);
    return;
  }

  // TODO(https://github.com/brave/brave-browser/issues/14015): Refactor this
  // logic to only apply permission rules if the new tab page ad was not served
  // by the library. |Serving::MaybeServeAd| is responsible for applying
  // permission rules for new tab page ads served by the library
  new_tab_page_ads::PermissionRules permission_rules;
  if (event_type == mojom::NewTabPageAdEventType::kViewed &&
      !permission_rules.HasPermission()) {
    BLOG(1, "New tab page ad: Not allowed due to permission rules");
    NotifyNewTabPageAdEventFailed(placement_id, creative_instance_id,
                                  event_type);
    return;
  }

  database::table::CreativeNewTabPageAds database_table;
  database_table.GetForCreativeInstanceId(
      creative_instance_id,
      [=](const bool success, const std::string& creative_instance_id,
          const CreativeNewTabPageAdInfo& creative_ad) {
        if (!success) {
          BLOG(1,
               "Failed to fire new tab page ad event due to missing creative "
               "instance id "
                   << creative_instance_id);
          NotifyNewTabPageAdEventFailed(placement_id, creative_instance_id,
                                        event_type);
          return;
        }

        const NewTabPageAdInfo& ad =
            BuildNewTabPageAd(creative_ad, placement_id);

        FireEvent(ad, placement_id, creative_instance_id, event_type);
      });
}

///////////////////////////////////////////////////////////////////////////////

void NewTabPageAd::FireEvent(const NewTabPageAdInfo& ad,
                             const std::string& placement_id,
                             const std::string& creative_instance_id,
                             const mojom::NewTabPageAdEventType event_type) {
  database::table::AdEvents database_table;
  database_table.GetForType(
      mojom::AdType::kNewTabPageAd,
      [=](const bool success, const AdEventList& ad_events) {
        if (!success) {
          BLOG(1, "New tab page ad: Failed to get ad events");
          NotifyNewTabPageAdEventFailed(placement_id, creative_instance_id,
                                        event_type);
          return;
        }

        if (ShouldDebounceAdEvent(ad, ad_events, event_type)) {
          BLOG(1, "New tab page ad: Not allowed as already fired "
                      << event_type << " event for this placement id "
                      << placement_id);
          NotifyNewTabPageAdEventFailed(placement_id, creative_instance_id,
                                        event_type);
          return;
        }

        if (event_type == mojom::NewTabPageAdEventType::kViewed) {
          // TODO(https://github.com/brave/brave-browser/issues/14015): We need
          // to fire an ad served event until new tab page ads are served by the
          // ads library
          FireEvent(placement_id, creative_instance_id,
                    mojom::NewTabPageAdEventType::kServed);
        }

        const auto ad_event =
            new_tab_page_ads::AdEventFactory::Build(event_type);
        ad_event->FireEvent(ad);

        NotifyNewTabPageAdEvent(ad, event_type);
      });
}

void NewTabPageAd::NotifyNewTabPageAdEvent(
    const NewTabPageAdInfo& ad,
    const mojom::NewTabPageAdEventType event_type) const {
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
    const std::string& placement_id,
    const std::string& creative_instance_id,
    const mojom::NewTabPageAdEventType event_type) const {
  for (NewTabPageAdObserver& observer : observers_) {
    observer.OnNewTabPageAdEventFailed(placement_id, creative_instance_id,
                                       event_type);
  }
}

}  // namespace ads
