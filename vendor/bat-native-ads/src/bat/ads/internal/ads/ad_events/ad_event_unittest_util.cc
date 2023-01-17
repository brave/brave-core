/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/ads/ad_events/ad_event_unittest_util.h"

#include <vector>

#include "base/check_op.h"
#include "base/functional/bind.h"
#include "base/guid.h"
#include "base/time/time.h"
#include "bat/ads/ad_info.h"
#include "bat/ads/ad_type.h"
#include "bat/ads/confirmation_type.h"
#include "bat/ads/internal/ads/ad_events/ad_event_info.h"
#include "bat/ads/internal/ads/ad_events/ad_events.h"
#include "bat/ads/internal/ads_client_helper.h"
#include "bat/ads/internal/common/instance_id_constants.h"
#include "bat/ads/internal/common/unittest/unittest_time_util.h"
#include "bat/ads/internal/creatives/creative_ad_info.h"

namespace ads {

AdEventInfo BuildAdEvent(const CreativeAdInfo& creative_ad,
                         const AdType& ad_type,
                         const ConfirmationType& confirmation_type,
                         const base::Time created_at) {
  AdEventInfo ad_event;
  ad_event.type = ad_type;
  ad_event.confirmation_type = confirmation_type;
  ad_event.placement_id = base::GUID::GenerateRandomV4().AsLowercaseString();
  ad_event.campaign_id = creative_ad.campaign_id;
  ad_event.creative_set_id = creative_ad.creative_set_id;
  ad_event.creative_instance_id = creative_ad.creative_instance_id;
  ad_event.advertiser_id = creative_ad.advertiser_id;
  ad_event.created_at = created_at;

  return ad_event;
}

AdEventInfo BuildAdEvent(const CreativeAdInfo& creative_ad,
                         const AdType& ad_type,
                         const ConfirmationType& confirmation_type) {
  return BuildAdEvent(creative_ad, ad_type, confirmation_type, Now());
}

AdEventInfo BuildAdEvent(const AdInfo& ad,
                         const AdType& ad_type,
                         const ConfirmationType& confirmation_type,
                         const base::Time created_at) {
  AdEventInfo ad_event;
  ad_event.type = ad_type;
  ad_event.confirmation_type = confirmation_type;
  ad_event.placement_id = base::GUID::GenerateRandomV4().AsLowercaseString();
  ad_event.campaign_id = ad.campaign_id;
  ad_event.creative_set_id = ad.creative_set_id;
  ad_event.creative_instance_id = ad.creative_instance_id;
  ad_event.advertiser_id = ad.advertiser_id;
  ad_event.created_at = created_at;

  return ad_event;
}

AdEventInfo BuildAdEvent(const AdInfo& ad,
                         const AdType& ad_type,
                         const ConfirmationType& confirmation_type) {
  return BuildAdEvent(ad, ad_type, confirmation_type, Now());
}

AdEventInfo BuildAdEvent(const std::string& placement_id,
                         const std::string& creative_set_id,
                         const ConfirmationType& confirmation_type) {
  AdEventInfo ad_event;

  ad_event.type = AdType::kNotificationAd;
  ad_event.confirmation_type = confirmation_type;
  ad_event.placement_id = placement_id;
  ad_event.campaign_id = "604df73f-bc6e-4583-a56d-ce4e243c8537";
  ad_event.creative_set_id = creative_set_id;
  ad_event.creative_instance_id = "7a3b6d9f-d0b7-4da6-8988-8d5b8938c94f";
  ad_event.advertiser_id = "f646c5f5-027a-4a35-b081-fce85e830b19";
  ad_event.created_at = Now();

  return ad_event;
}

AdEventInfo BuildAdEvent(const std::string& creative_set_id,
                         const ConfirmationType& confirmation_type) {
  const std::string uuid = base::GUID::GenerateRandomV4().AsLowercaseString();
  return BuildAdEvent(uuid, creative_set_id, confirmation_type);
}

void RecordAdEvent(const AdType& type,
                   const ConfirmationType& confirmation_type) {
  RecordAdEvents(type, confirmation_type, 1);
}

void RecordAdEvents(const AdType& type,
                    const ConfirmationType& confirmation_type,
                    const int count) {
  DCHECK_GT(count, 0);

  const std::string& id = GetInstanceId();
  const std::string ad_type_as_string = type.ToString();
  const std::string confirmation_type_as_string = confirmation_type.ToString();
  const base::Time time = Now();

  for (int i = 0; i < count; i++) {
    AdsClientHelper::GetInstance()->RecordAdEventForId(
        id, ad_type_as_string, confirmation_type_as_string, time);
  }
}

void FireAdEvent(const AdEventInfo& ad_event) {
  LogAdEvent(ad_event,
             base::BindOnce([](const bool success) { CHECK(success); }));
}

void FireAdEvents(const AdEventInfo& ad_event, const int count) {
  for (int i = 0; i < count; i++) {
    FireAdEvent(ad_event);
  }
}

int GetAdEventCount(const AdType& ad_type,
                    const ConfirmationType& confirmation_type) {
  const std::vector<base::Time> ad_events =
      GetAdEventHistory(ad_type, confirmation_type);
  return ad_events.size();
}

}  // namespace ads
