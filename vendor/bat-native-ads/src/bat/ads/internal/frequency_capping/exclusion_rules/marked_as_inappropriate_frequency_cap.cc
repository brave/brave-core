/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/frequency_capping/exclusion_rules/marked_as_inappropriate_frequency_cap.h"

#include "base/strings/stringprintf.h"
#include "bat/ads/internal/bundle/creative_ad_info.h"
#include "bat/ads/internal/client/client.h"
#include "bat/ads/internal/client/preferences/flagged_ad_info.h"

namespace ads {

MarkedAsInappropriateFrequencyCap::MarkedAsInappropriateFrequencyCap() =
    default;

MarkedAsInappropriateFrequencyCap::~MarkedAsInappropriateFrequencyCap() =
    default;

bool MarkedAsInappropriateFrequencyCap::ShouldExclude(
    const CreativeAdInfo& ad) {
  if (!DoesRespectCap(ad)) {
    last_message_ = base::StringPrintf(
        "creativeSetId %s excluded due to being marked as inappropriate",
        ad.creative_set_id.c_str());

    return true;
  }

  return false;
}

std::string MarkedAsInappropriateFrequencyCap::get_last_message() const {
  return last_message_;
}

bool MarkedAsInappropriateFrequencyCap::DoesRespectCap(
    const CreativeAdInfo& ad) {
  const FlaggedAdList flagged_ads = Client::Get()->get_flagged_ads();
  if (flagged_ads.empty()) {
    return true;
  }

  const auto iter =
      std::find_if(flagged_ads.begin(), flagged_ads.end(),
                   [&ad](const FlaggedAdInfo& flagged_ad) {
                     return flagged_ad.creative_set_id == ad.creative_set_id;
                   });

  if (iter == flagged_ads.end()) {
    return true;
  }

  return false;
}

}  // namespace ads
