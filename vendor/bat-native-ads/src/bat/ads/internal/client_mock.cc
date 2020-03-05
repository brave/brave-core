/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <memory>
#include <string>

#include "bat/ads/internal/client_mock.h"

#include "bat/ads/ad_history.h"
#include "bat/ads/internal/time.h"
#include "base/guid.h"

namespace ads {

void ClientMock::GeneratePastAdHistoryFromNow(
    const std::string uuid,
    const int64_t time_offset_per_ad_in_seconds,
    const uint8_t count) {
  auto now_in_seconds = Time::NowInSeconds();

  auto ad_history = std::make_unique<AdHistory>();
  ad_history->uuid = base::GenerateGUID();
  ad_history->ad_content.uuid = uuid;
  ad_history->ad_content.ad_action = ConfirmationType::VIEW;

  for (uint8_t i = 0; i < count; i++) {
    now_in_seconds -= time_offset_per_ad_in_seconds;

    ad_history->timestamp_in_seconds = now_in_seconds;
    AppendAdHistoryToAdsShownHistory(*ad_history);
  }
}

void ClientMock::GeneratePastCreativeSetHistoryFromNow(
    const std::string& creative_set_id,
    const int64_t time_offset_per_ad_in_seconds,
    const uint8_t count) {
  auto now_in_seconds = Time::NowInSeconds();

  for (uint8_t i = 0; i < count; i++) {
    now_in_seconds -= time_offset_per_ad_in_seconds;

    AppendTimestampToCreativeSetHistoryForUuid(creative_set_id, now_in_seconds);
  }
}

void ClientMock::GeneratePastCampaignHistoryFromNow(
    const std::string campaign_id,
    const int64_t time_offset_per_ad_in_seconds,
    const uint8_t count) {
  auto now_in_seconds = Time::NowInSeconds();

  for (uint8_t i = 0; i < count; i++) {
    now_in_seconds -= time_offset_per_ad_in_seconds;

    AppendTimestampToCampaignHistoryForUuid(campaign_id, now_in_seconds);
  }
}

}  // namespace ads
