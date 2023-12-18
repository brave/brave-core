/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/user_engagement/ad_events/ad_event_unittest_util.h"

#include <string>

#include "base/check_op.h"
#include "base/functional/bind.h"
#include "base/time/time.h"
#include "brave/components/brave_ads/core/internal/ad_units/ad_unittest_constants.h"
#include "brave/components/brave_ads/core/internal/ad_units/ad_unittest_util.h"
#include "brave/components/brave_ads/core/internal/client/ads_client_util.h"
#include "brave/components/brave_ads/core/internal/common/instance_id.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_time_util.h"
#include "brave/components/brave_ads/core/internal/creatives/creative_ad_info.h"
#include "brave/components/brave_ads/core/internal/user_engagement/ad_events/ad_event_info.h"
#include "brave/components/brave_ads/core/internal/user_engagement/ad_events/ad_events.h"

namespace brave_ads::test {

AdEventInfo BuildAdEvent(const CreativeAdInfo& creative_ad,
                         const AdType ad_type,
                         const ConfirmationType confirmation_type,
                         const base::Time created_at,
                         const bool should_use_random_uuids) {
  AdEventInfo ad_event;

  ad_event.type = ad_type;
  ad_event.confirmation_type = confirmation_type;
  ad_event.placement_id = GetConstantId(should_use_random_uuids, kPlacementId);
  ad_event.campaign_id = creative_ad.campaign_id;
  ad_event.creative_set_id = creative_ad.creative_set_id;
  ad_event.creative_instance_id = creative_ad.creative_instance_id;
  ad_event.advertiser_id = creative_ad.advertiser_id;
  ad_event.segment = creative_ad.segment;
  ad_event.created_at = created_at;

  return ad_event;
}

void RecordAdEvent(const AdType ad_type,
                   const ConfirmationType confirmation_type) {
  RecordAdEvents(ad_type, confirmation_type, /*count=*/1);
}

void RecordAdEvents(const AdType ad_type,
                    const ConfirmationType confirmation_type,
                    const int count) {
  CHECK_GT(count, 0);

  const std::string& id = GetInstanceId();

  for (int i = 0; i < count; ++i) {
    CacheAdEventForInstanceId(id, ad_type, confirmation_type, Now());
  }
}

void RecordAdEvent(const AdEventInfo& ad_event) {
  RecordAdEvent(ad_event,
                base::BindOnce([](const bool success) { CHECK(success); }));
}

void RecordAdEvents(const AdEventInfo& ad_event, const int count) {
  for (int i = 0; i < count; ++i) {
    RecordAdEvent(ad_event);
  }
}

}  // namespace brave_ads::test
