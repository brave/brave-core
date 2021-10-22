/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/frequency_capping/exclusion_rules/transferred_frequency_cap.h"

#include <algorithm>
#include <iterator>

#include "base/strings/stringprintf.h"
#include "bat/ads/internal/frequency_capping/frequency_capping_features.h"
#include "bat/ads/internal/frequency_capping/frequency_capping_util.h"

namespace ads {

namespace {
const int kTransferredFrequencyCap = 1;
}  // namespace

TransferredFrequencyCap::TransferredFrequencyCap(const AdEventList& ad_events)
    : ad_events_(ad_events) {}

TransferredFrequencyCap::~TransferredFrequencyCap() = default;

std::string TransferredFrequencyCap::GetUuid(
    const CreativeAdInfo& creative_ad) const {
  return creative_ad.campaign_id;
}

bool TransferredFrequencyCap::ShouldExclude(const CreativeAdInfo& creative_ad) {
  if (!DoesRespectCap(ad_events_, creative_ad)) {
    last_message_ = base::StringPrintf(
        "campaignId %s has exceeded the transferred frequency cap",
        creative_ad.campaign_id.c_str());

    return true;
  }

  return false;
}

std::string TransferredFrequencyCap::GetLastMessage() const {
  return last_message_;
}

bool TransferredFrequencyCap::DoesRespectCap(
    const AdEventList& ad_events,
    const CreativeAdInfo& creative_ad) {
  const base::Time now = base::Time::Now();

  const base::TimeDelta time_constraint =
      features::frequency_capping::ExcludeAdIfTransferredWithinTimeWindow();

  const int count = std::count_if(
      ad_events.cbegin(), ad_events.cend(),
      [&now, &time_constraint, &creative_ad](const AdEventInfo& ad_event) {
        return ad_event.confirmation_type == ConfirmationType::kTransferred &&
               ad_event.campaign_id == creative_ad.campaign_id &&
               now - ad_event.created_at < time_constraint &&
               DoesAdTypeSupportFrequencyCapping(ad_event.type);
      });

  if (count >= kTransferredFrequencyCap) {
    return false;
  }

  return true;
}

}  // namespace ads
