/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/frequency_capping/exclusion_rules/marked_to_no_longer_receive_frequency_cap.h"

#include <algorithm>

#include "base/strings/stringprintf.h"
#include "bat/ads/internal/ad_targeting/ad_targeting_segment_util.h"
#include "bat/ads/internal/bundle/creative_ad_info.h"
#include "bat/ads/internal/client/client.h"
#include "bat/ads/internal/client/preferences/filtered_category_info.h"

namespace ads {

MarkedToNoLongerReceiveFrequencyCap::MarkedToNoLongerReceiveFrequencyCap() =
    default;

MarkedToNoLongerReceiveFrequencyCap::~MarkedToNoLongerReceiveFrequencyCap() =
    default;

bool MarkedToNoLongerReceiveFrequencyCap::ShouldExclude(
    const CreativeAdInfo& ad) {
  if (!DoesRespectCap(ad)) {
    last_message_ = base::StringPrintf(
        "creativeSetId %s excluded due to being marked to no longer receive "
        "ads",
        ad.creative_set_id.c_str());

    return true;
  }

  return false;
}

std::string MarkedToNoLongerReceiveFrequencyCap::get_last_message() const {
  return last_message_;
}

bool MarkedToNoLongerReceiveFrequencyCap::DoesRespectCap(
    const CreativeAdInfo& ad) {
  const FilteredCategoryList filtered_categories =
      Client::Get()->get_filtered_categories();
  if (filtered_categories.empty()) {
    return true;
  }

  if (ShouldFilterSegment(ad.segment)) {
    return false;
  }

  return true;
}

}  // namespace ads
