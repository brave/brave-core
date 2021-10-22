/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/ad_events/ad_event_unittest_util.h"

#include "base/guid.h"
#include "base/time/time.h"
#include "bat/ads/confirmation_type.h"
#include "bat/ads/internal/ad_events/ad_event_info.h"
#include "bat/ads/internal/ad_events/ad_events.h"
#include "bat/ads/internal/bundle/creative_ad_info.h"
#include "bat/ads/internal/unittest_time_util.h"
#include "bat/ads/internal/unittest_util.h"

namespace ads {

AdEventInfo BuildAdEvent(const CreativeAdInfo& creative_ad,
                         const ConfirmationType confirmation_type,
                         const base::Time& created_at) {
  AdEventInfo ad_event;
  ad_event.uuid = base::GenerateGUID();
  ad_event.confirmation_type = confirmation_type;
  ad_event.creative_instance_id = creative_ad.creative_instance_id;
  ad_event.advertiser_id = creative_ad.advertiser_id;
  ad_event.created_at = created_at;

  return ad_event;
}

AdEventInfo BuildAdEvent(const ConfirmationType confirmation_type) {
  AdEventInfo ad_event;

  ad_event.type = AdType::kAdNotification;
  ad_event.uuid = base::GenerateGUID();
  ad_event.confirmation_type = confirmation_type;
  ad_event.campaign_id = base::GenerateGUID();
  ad_event.creative_set_id = base::GenerateGUID();
  ad_event.creative_instance_id = base::GenerateGUID();
  ad_event.advertiser_id = base::GenerateGUID();
  ad_event.created_at = Now();

  return ad_event;
}

AdEventInfo BuildAdEvent(const std::string& uuid,
                         const std::string& creative_set_id,
                         const ConfirmationType confirmation_type) {
  AdEventInfo ad_event;

  ad_event.type = AdType::kAdNotification;
  ad_event.uuid = uuid;
  ad_event.confirmation_type = confirmation_type;
  ad_event.campaign_id = "604df73f-bc6e-4583-a56d-ce4e243c8537";
  ad_event.creative_set_id = creative_set_id;
  ad_event.creative_instance_id = "7a3b6d9f-d0b7-4da6-8988-8d5b8938c94f";
  ad_event.advertiser_id = "f646c5f5-027a-4a35-b081-fce85e830b19";
  ad_event.created_at = Now();

  return ad_event;
}

AdEventInfo BuildAdEvent(const std::string& creative_set_id,
                         const ConfirmationType confirmation_type) {
  const std::string uuid = base::GenerateGUID();
  return BuildAdEvent(uuid, creative_set_id, confirmation_type);
}

void FireAdEvent(const AdEventInfo& ad_event) {
  LogAdEvent(ad_event, [](const bool success) { ASSERT_TRUE(success); });
}

}  // namespace ads
