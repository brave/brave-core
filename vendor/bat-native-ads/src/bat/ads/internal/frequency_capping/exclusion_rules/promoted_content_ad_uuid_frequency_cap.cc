/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/frequency_capping/exclusion_rules/promoted_content_ad_uuid_frequency_cap.h"

#include <algorithm>
#include <iterator>

#include "base/strings/stringprintf.h"

namespace ads {

namespace {
const int kPromotedContentAdUuidFrequencyCap = 1;
}  // namespace

PromotedContentAdUuidFrequencyCap::PromotedContentAdUuidFrequencyCap(
    const AdEventList& ad_events)
    : ad_events_(ad_events) {}

PromotedContentAdUuidFrequencyCap::~PromotedContentAdUuidFrequencyCap() =
    default;

std::string PromotedContentAdUuidFrequencyCap::GetUuid(const AdInfo& ad) const {
  return ad.uuid;
}

bool PromotedContentAdUuidFrequencyCap::ShouldExclude(const AdInfo& ad) {
  if (!DoesRespectCap(ad_events_, ad)) {
    last_message_ = base::StringPrintf(
        "uuid %s has exceeded the promoted content ad frequency cap",
        ad.uuid.c_str());

    return true;
  }

  return false;
}

std::string PromotedContentAdUuidFrequencyCap::GetLastMessage() const {
  return last_message_;
}

bool PromotedContentAdUuidFrequencyCap::DoesRespectCap(
    const AdEventList& ad_events,
    const AdInfo& ad) {
  const int count = std::count_if(
      ad_events.cbegin(), ad_events.cend(), [&ad](const AdEventInfo& ad_event) {
        return ad_event.type == AdType::kPromotedContentAd &&
               ad_event.uuid == ad.uuid &&
               ad_event.confirmation_type == ConfirmationType::kViewed;
      });

  if (count >= kPromotedContentAdUuidFrequencyCap) {
    return false;
  }

  return true;
}

}  // namespace ads
