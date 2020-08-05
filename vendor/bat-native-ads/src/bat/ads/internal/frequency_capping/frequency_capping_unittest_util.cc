/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/frequency_capping/frequency_capping_unittest_util.h"

#include "base/guid.h"
#include "bat/ads/ad_history.h"
#include "bat/ads/internal/ads_impl.h"
#include "bat/ads/internal/time_util.h"

namespace ads {

void GeneratePastCreativeSetHistoryFromNow(
    const std::unique_ptr<AdsImpl>& ads,
    const std::string& creative_set_id,
    const uint64_t time_offset_in_seconds,
    const int count) {
  uint64_t timestamp_in_seconds =
      static_cast<uint64_t>(base::Time::Now().ToDoubleT());

  for (int i = 0; i < count; i++) {
    timestamp_in_seconds -= time_offset_in_seconds;

    ads->get_client()->AppendTimestampToCreativeSetHistory(
        creative_set_id, timestamp_in_seconds);
  }
}

void GeneratePastCampaignHistoryFromNow(
    const std::unique_ptr<AdsImpl>& ads,
    const std::string& campaign_id,
    const uint64_t time_offset_in_seconds,
    const int count) {
  uint64_t timestamp_in_seconds =
      static_cast<uint64_t>(base::Time::Now().ToDoubleT());

  for (int i = 0; i < count; i++) {
    timestamp_in_seconds -= time_offset_in_seconds;

    ads->get_client()->AppendTimestampToCampaignHistory(
        campaign_id, timestamp_in_seconds);
  }
}

void GeneratePastAdsHistoryFromNow(
    const std::unique_ptr<AdsImpl>& ads,
    const std::string& creative_instance_id,
    const uint64_t time_offset_in_seconds,
    const int count) {
  uint64_t now_in_seconds =
      static_cast<uint64_t>(base::Time::Now().ToDoubleT());

  AdHistory history;
  history.uuid = base::GenerateGUID();
  history.ad_content.creative_instance_id = creative_instance_id;
  history.ad_content.ad_action = ConfirmationType::kViewed;

  for (int i = 0; i < count; i++) {
    now_in_seconds -= time_offset_in_seconds;
    history.timestamp_in_seconds = now_in_seconds;

    ads->get_client()->AppendAdHistoryToAdsHistory(history);
  }
}

void GeneratePastAdConversionHistoryFromNow(
    const std::unique_ptr<AdsImpl>& ads,
    const std::string& creative_set_id,
    const uint64_t time_offset_in_seconds,
    const int count) {
  uint64_t timestamp_in_seconds =
      static_cast<uint64_t>(base::Time::Now().ToDoubleT());

  for (int i = 0; i < count; i++) {
    timestamp_in_seconds -= time_offset_in_seconds;

    ads->get_client()->AppendTimestampToAdConversionHistory(
        creative_set_id, timestamp_in_seconds);
  }
}

}  // namespace ads
