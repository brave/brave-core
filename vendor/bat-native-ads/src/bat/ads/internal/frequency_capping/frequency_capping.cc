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

std::deque<uint64_t> FrequencyCapping::GetCreativeSetHistory(
    const std::string& creative_set_id) const {
  std::deque<uint64_t> history;

  auto creative_set_history = client_->GetCreativeSetHistory();
  if (creative_set_history.find(creative_set_id)
      != creative_set_history.end()) {
    history = creative_set_history.at(creative_set_id);
  }

  return history;
}

std::deque<uint64_t> FrequencyCapping::GetAdsShownHistory() const {
  std::deque<uint64_t> history;

  const std::deque<AdHistory> ads_history = client_->GetAdsShownHistory();
  for (const auto& ad : ads_history) {
    if (ad.ad_content.ad_action != ConfirmationType::kViewed) {
      continue;
    }

    history.push_back(ad.timestamp_in_seconds);
  }

  return history;
}

std::deque<uint64_t> FrequencyCapping::GetAdsHistory(
    const std::string& creative_instance_id) const {
  std::deque<uint64_t> history;

  const std::deque<AdHistory> ads_history = client_->GetAdsShownHistory();
  for (const auto& ad : ads_history) {
    if (ad.ad_content.ad_action != ConfirmationType::kViewed ||
        ad.ad_content.creative_instance_id != creative_instance_id) {
      continue;
    }

    history.push_back(ad.timestamp_in_seconds);
  }

  return history;
}

std::deque<uint64_t> FrequencyCapping::GetCampaign(
    const std::string& campaign_id) const {
  std::deque<uint64_t> history;

  auto campaign_history = client_->GetCampaignHistory();
  if (campaign_history.find(campaign_id) != campaign_history.end()) {
    history = campaign_history.at(campaign_id);
  }

  return history;
}

std::deque<uint64_t> FrequencyCapping::GetAdConversionHistory(
    const std::string& creative_set_id) const {
  std::deque<uint64_t> history;

  auto creative_set_history = client_->GetAdConversionHistory();
  if (creative_set_history.find(creative_set_id)
      != creative_set_history.end()) {
    history = creative_set_history.at(creative_set_id);
  }

  return history;
}

}  // namespace ads
