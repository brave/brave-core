/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/frequency_capping/frequency_capping_unittest_utils.h"

#include "bat/ads/ad_history.h"
#include "bat/ads/internal/client.h"
#include "bat/ads/purchase_intent_signal_history.h"

#include "base/guid.h"
#include "base/time/time.h"

namespace ads {

void GeneratePastCreativeSetHistoryFromNow(
    Client* client,
    const std::string& creative_set_id,
    const int64_t time_offset_in_seconds,
    const uint8_t count) {
  uint64_t timestamp_in_seconds =
      static_cast<uint64_t>(base::Time::Now().ToDoubleT());

  for (uint8_t i = 0; i < count; i++) {
    timestamp_in_seconds -= time_offset_in_seconds;

    client->AppendTimestampToCreativeSetHistory(creative_set_id,
        timestamp_in_seconds);
  }
}

void GeneratePastCampaignHistoryFromNow(
    Client* client,
    const std::string& campaign_id,
    const int64_t time_offset_in_seconds,
    const uint8_t count) {
  uint64_t timestamp_in_seconds =
      static_cast<uint64_t>(base::Time::Now().ToDoubleT());

  for (uint8_t i = 0; i < count; i++) {
    timestamp_in_seconds -= time_offset_in_seconds;

    client->AppendTimestampToCampaignHistory(campaign_id, timestamp_in_seconds);
  }
}

void GeneratePastAdsHistoryFromNow(
    Client* client,
    const std::string& creative_instance_id,
    const int64_t time_offset_in_seconds,
    const uint8_t count) {
  uint64_t now_in_seconds =
      static_cast<uint64_t>(base::Time::Now().ToDoubleT());

  AdHistory history;
  history.uuid = base::GenerateGUID();
  history.ad_content.creative_instance_id = creative_instance_id;
  history.ad_content.ad_action = ConfirmationType::kViewed;

  for (uint8_t i = 0; i < count; i++) {
    now_in_seconds -= time_offset_in_seconds;
    history.timestamp_in_seconds = now_in_seconds;

    client->AppendAdHistoryToAdsHistory(history);
  }
}

void GeneratePastAdConversionHistoryFromNow(
    Client* client,
    const std::string& creative_set_id,
    const int64_t time_offset_in_seconds,
    const uint8_t count) {
  uint64_t timestamp_in_seconds =
      static_cast<uint64_t>(base::Time::Now().ToDoubleT());

  for (uint8_t i = 0; i < count; i++) {
    timestamp_in_seconds -= time_offset_in_seconds;

    client->AppendTimestampToAdConversionHistory(creative_set_id,
        timestamp_in_seconds);
  }
}

}  // namespace ads
