/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/user/user_interaction/ad_events/ad_event_unittest_util.h"

#include <string>
#include <vector>

#include "base/check_op.h"
#include "base/functional/bind.h"
#include "base/time/time.h"
#include "base/uuid.h"
#include "brave/components/brave_ads/core/internal/client/ads_client_helper.h"
#include "brave/components/brave_ads/core/internal/common/instance_id_constants.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_time_util.h"
#include "brave/components/brave_ads/core/internal/creatives/creative_ad_info.h"
#include "brave/components/brave_ads/core/internal/units/ad_unittest_constants.h"
#include "brave/components/brave_ads/core/internal/user/user_interaction/ad_events/ad_event_info.h"
#include "brave/components/brave_ads/core/internal/user/user_interaction/ad_events/ad_events.h"
#include "brave/components/brave_ads/core/public/account/confirmations/confirmation_type.h"
#include "brave/components/brave_ads/core/public/units/ad_type.h"

namespace brave_ads {

AdEventInfo BuildAdEventForTesting(const CreativeAdInfo& creative_ad,
                                   const AdType& ad_type,
                                   const ConfirmationType& confirmation_type,
                                   const base::Time created_at,
                                   const bool should_use_random_uuids) {
  AdEventInfo ad_event;

  ad_event.type = ad_type;
  ad_event.confirmation_type = confirmation_type;
  ad_event.placement_id =
      should_use_random_uuids
          ? base::Uuid::GenerateRandomV4().AsLowercaseString()
          : kPlacementId;
  ad_event.campaign_id = creative_ad.campaign_id;
  ad_event.creative_set_id = creative_ad.creative_set_id;
  ad_event.creative_instance_id = creative_ad.creative_instance_id;
  ad_event.advertiser_id = creative_ad.advertiser_id;
  ad_event.segment = creative_ad.segment;
  ad_event.created_at = created_at;

  return ad_event;
}

void RecordAdEventForTesting(const AdType& type,
                             const ConfirmationType& confirmation_type) {
  RecordAdEventsForTesting(type, confirmation_type, /*count*/ 1);
}

void RecordAdEventsForTesting(const AdType& type,
                              const ConfirmationType& confirmation_type,
                              const int count) {
  CHECK_GT(count, 0);

  const std::string& id = GetInstanceId();
  const std::string ad_type_as_string = type.ToString();
  const std::string confirmation_type_as_string = confirmation_type.ToString();
  const base::Time time = Now();

  for (int i = 0; i < count; i++) {
    AdsClientHelper::GetInstance()->RecordAdEventForId(
        id, ad_type_as_string, confirmation_type_as_string, time);
  }
}

void FireAdEventForTesting(const AdEventInfo& ad_event) {
  LogAdEvent(ad_event,
             base::BindOnce([](const bool success) { CHECK(success); }));
}

void FireAdEventsForTesting(const AdEventInfo& ad_event, const size_t count) {
  for (size_t i = 0; i < count; i++) {
    FireAdEventForTesting(ad_event);
  }
}

size_t GetAdEventCountForTesting(const AdType& ad_type,
                                 const ConfirmationType& confirmation_type) {
  const std::vector<base::Time> ad_events =
      GetAdEventHistory(ad_type, confirmation_type);
  return ad_events.size();
}

}  // namespace brave_ads
