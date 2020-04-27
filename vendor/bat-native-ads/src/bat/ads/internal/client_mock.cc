/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <memory>
#include <string>

#include "bat/ads/internal/client_mock.h"

#include "bat/ads/ad_history.h"
#include "bat/ads/purchase_intent_signal_history.h"
#include "bat/ads/internal/time.h"
#include "base/guid.h"

namespace ads {

void ClientMock::GeneratePastAdHistoryFromNow(
    const std::string& creative_instance_id,
    const int64_t time_offset_per_ad_in_seconds,
    const uint8_t count) {
  uint64_t now_in_seconds = Time::NowInSeconds();

  AdHistory ad_history;
  ad_history.uuid = base::GenerateGUID();
  ad_history.ad_content.creative_instance_id = creative_instance_id;
  ad_history.ad_content.ad_action = ConfirmationType::kViewed;

  for (uint8_t i = 0; i < count; i++) {
    now_in_seconds -= time_offset_per_ad_in_seconds;

    ad_history.timestamp_in_seconds = now_in_seconds;
    AppendAdHistoryToAdsShownHistory(ad_history);
  }
}

void ClientMock::GeneratePastCreativeSetHistoryFromNow(
    const std::string& creative_set_id,
    const int64_t time_offset_per_ad_in_seconds,
    const uint8_t count) {
  uint64_t now_in_seconds = Time::NowInSeconds();

  for (uint8_t i = 0; i < count; i++) {
    now_in_seconds -= time_offset_per_ad_in_seconds;

    AppendTimestampToCreativeSetHistory(creative_set_id, now_in_seconds);
  }
}

void ClientMock::GeneratePastCampaignHistoryFromNow(
    const std::string& campaign_id,
    const int64_t time_offset_per_ad_in_seconds,
    const uint8_t count) {
  uint64_t now_in_seconds = Time::NowInSeconds();

  for (uint8_t i = 0; i < count; i++) {
    now_in_seconds -= time_offset_per_ad_in_seconds;

    AppendTimestampToCampaignHistory(campaign_id, now_in_seconds);
  }
}

void ClientMock::GeneratePastPurchaseIntentSignalHistoryFromNow(
    const std::string& segment,
    const uint64_t time_offset_in_seconds,
    const uint16_t weight) {
  uint64_t now_in_seconds = Time::NowInSeconds();
  now_in_seconds -= time_offset_in_seconds;
  PurchaseIntentSignalHistory history;
  history.timestamp_in_seconds = 0;
  history.weight = weight;
  AppendToPurchaseIntentSignalHistoryForSegment(segment, history);
}

void ClientMock::GeneratePastAdConversionHistoryFromNow(
    const std::string& creative_set_id,
    const int64_t time_offset_per_ad_in_seconds,
    const uint8_t count) {
  uint64_t now_in_seconds = Time::NowInSeconds();

  for (uint8_t i = 0; i < count; i++) {
    now_in_seconds -= time_offset_per_ad_in_seconds;

    AppendTimestampToAdConversionHistory(creative_set_id, now_in_seconds);
  }
}

}  // namespace ads
