/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/ad_events/ad_event_unittest_util.h"

#include <algorithm>

#include "base/check_op.h"
#include "base/guid.h"
#include "base/time/time.h"
#include "bat/ads/ad_info.h"
#include "bat/ads/ad_type.h"
#include "bat/ads/confirmation_type.h"
#include "bat/ads/internal/ad_events/ad_event_info.h"
#include "bat/ads/internal/ad_events/ad_events.h"
#include "bat/ads/internal/ad_server/catalog/bundle/creative_ad_info.h"
#include "bat/ads/internal/ads_client_helper.h"
#include "bat/ads/internal/base/instance_id_util.h"
#include "bat/ads/internal/base/unittest_time_util.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace ads {

AdEventInfo BuildAdEvent(const CreativeAdInfo& creative_ad,
                         const AdType& ad_type,
                         const ConfirmationType& confirmation_type,
                         const base::Time created_at) {
  AdEventInfo ad_event;
  ad_event.type = ad_type;
  ad_event.confirmation_type = confirmation_type;
  ad_event.uuid = base::GUID::GenerateRandomV4().AsLowercaseString();
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
  ad_event.uuid = base::GUID::GenerateRandomV4().AsLowercaseString();
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

AdEventInfo BuildAdEvent(const std::string& uuid,
                         const std::string& creative_set_id,
                         const ConfirmationType& confirmation_type) {
  AdEventInfo ad_event;

  ad_event.type = AdType::kAdNotification;
  ad_event.confirmation_type = confirmation_type;
  ad_event.uuid = uuid;
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
  const std::string& ad_type_as_string = type.ToString();
  const std::string& confirmation_type_as_string = confirmation_type.ToString();
  const base::Time time = Now();

  for (int i = 0; i < count; i++) {
    AdsClientHelper::Get()->RecordAdEventForId(
        id, ad_type_as_string, confirmation_type_as_string, time);
  }
}

void FireAdEvent(const AdEventInfo& ad_event) {
  LogAdEvent(ad_event, [](const bool success) { ASSERT_TRUE(success); });
}

void FireAdEvents(const AdEventInfo& ad_event, const int count) {
  for (int i = 0; i < count; i++) {
    FireAdEvent(ad_event);

    AdEventInfo served_ad_event = ad_event;
    served_ad_event.confirmation_type = ConfirmationType::kServed;
    FireAdEvent(served_ad_event);
  }
}

int GetAdEventCount(const AdType& ad_type,
                    const ConfirmationType& confirmation_type,
                    const AdEventList& ad_events) {
  return std::count_if(
      ad_events.cbegin(), ad_events.cend(),
      [&ad_type, &confirmation_type](const AdEventInfo& ad_event) {
        return ad_event.type == ad_type &&
               ad_event.confirmation_type == confirmation_type;
      });
}

}  // namespace ads
