/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/frequency_capping/frequency_capping_unittest_util.h"

#include <cstdint>
#include <string>

#include "base/guid.h"
#include "base/time/time.h"
#include "bat/ads/internal/ads_client_helper.h"
#include "bat/ads/internal/client/client.h"
#include "bat/ads/internal/database/tables/ad_events_database_table_unittest_util.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace ads {

AdEventInfo GenerateAdEvent(const AdType& type,
                            const CreativeAdInfo& ad,
                            const ConfirmationType& confirmation_type) {
  AdEventInfo ad_event;

  ad_event.uuid = base::GenerateGUID();
  ad_event.type = type;
  ad_event.confirmation_type = confirmation_type;
  ad_event.campaign_id = ad.campaign_id;
  ad_event.creative_set_id = ad.creative_set_id;
  ad_event.creative_instance_id = ad.creative_instance_id;
  ad_event.advertiser_id = ad.advertiser_id;
  ad_event.timestamp = static_cast<int64_t>(base::Time::Now().ToDoubleT());

  return ad_event;
}

AdEventInfo GenerateAdEvent(const AdType& type,
                            const AdInfo& ad,
                            const ConfirmationType& confirmation_type) {
  AdEventInfo ad_event;

  ad_event.uuid = ad.uuid;
  ad_event.type = type;
  ad_event.confirmation_type = confirmation_type;
  ad_event.campaign_id = ad.campaign_id;
  ad_event.creative_set_id = ad.creative_set_id;
  ad_event.creative_instance_id = ad.creative_instance_id;
  ad_event.advertiser_id = ad.advertiser_id;
  ad_event.timestamp = static_cast<int64_t>(base::Time::Now().ToDoubleT());

  return ad_event;
}

void RecordAdEvents(const AdType& type,
                    const ConfirmationType& confirmation_type,
                    const int count) {
  DCHECK_GT(count, 0);

  const std::string ad_type_as_string = std::string(type);

  const std::string confirmation_type_as_string =
      std::string(confirmation_type);

  const uint64_t timestamp =
      static_cast<int64_t>(base::Time::Now().ToDoubleT());

  for (int i = 0; i < count; i++) {
    AdsClientHelper::Get()->RecordAdEvent(
        ad_type_as_string, confirmation_type_as_string, timestamp);
  }
}

void RecordAdEvent(const AdType& type,
                   const ConfirmationType& confirmation_type) {
  RecordAdEvents(type, confirmation_type, 1);
}

void ResetFrequencyCaps(const AdType& type) {
  Client::Get()->ResetAllSeenAdsForType(type);

  Client::Get()->ResetAllSeenAdvertisersForType(type);

  database::table::ad_events::Reset(
      [](const Result result) { ASSERT_EQ(Result::SUCCESS, result); });
}

}  // namespace ads
