/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/frequency_capping/frequency_capping.h"
#include "bat/ads/creative_ad_notification_info.h"
#include "bat/ads/internal/client.h"
#include "bat/ads/internal/time.h"

namespace ads {

FrequencyCapping::FrequencyCapping(
    const Client* const client)
    : client_(client) {
}

FrequencyCapping::~FrequencyCapping() = default;

bool FrequencyCapping::DoesHistoryRespectCapForRollingTimeConstraint(
    const std::deque<uint64_t> history,
    const uint64_t time_constraint_in_seconds,
    const uint64_t cap) const {
  uint64_t count = 0;

  auto now_in_seconds = Time::NowInSeconds();

  for (const auto& timestamp_in_seconds : history) {
    if (now_in_seconds - timestamp_in_seconds < time_constraint_in_seconds) {
      count++;
    }
  }

  if (count < cap) {
    return true;
  }

  return false;
}

std::deque<uint64_t> FrequencyCapping::GetCreativeSetHistoryForUuid(
    const std::string& uuid) const {
  std::deque<uint64_t> history;

  auto creative_set_history = client_->GetCreativeSetHistory();
  if (creative_set_history.find(uuid) != creative_set_history.end()) {
     history = creative_set_history.at(uuid);
  }

  return history;
}

std::deque<uint64_t> FrequencyCapping::GetAdsShownHistory() const {
  std::deque<uint64_t> history;
  auto ads_history = client_->GetAdsShownHistory();

  for (const auto& detail : ads_history) {
    history.push_back(detail.timestamp_in_seconds);
  }

  return history;
}

std::deque<uint64_t> FrequencyCapping::GetAdsHistoryForUuid(
    const std::string& uuid) const {
  std::deque<uint64_t> history;
  auto ads_history = client_->GetAdsShownHistory();

  for (const auto& ad : ads_history) {
    if (ad.ad_content.uuid != uuid) {
      continue;
    }

    history.push_back(ad.timestamp_in_seconds);
  }

  return history;
}

std::deque<uint64_t> FrequencyCapping::GetCampaignForUuid(
    const std::string& uuid) const {
  std::deque<uint64_t> history;

  auto campaign_history = client_->GetCampaignHistory();
  if (campaign_history.find(uuid) != campaign_history.end()) {
    history = campaign_history.at(uuid);
  }

  return history;
}

}  // namespace ads
