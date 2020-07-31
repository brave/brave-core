/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/eligible_ads/eligible_ads_pacing_filter.h"

#include <algorithm>
#include <map>
#include <utility>

#include "base/rand_util.h"
#include "bat/ads/internal/logging.h"

namespace ads {

EligibleAdsPacingFilter::EligibleAdsPacingFilter() = default;

EligibleAdsPacingFilter::~EligibleAdsPacingFilter() = default;

CreativeAdNotificationList EligibleAdsPacingFilter::Apply(
    const CreativeAdNotificationList& ads) const {
  CreativeAdNotificationList paced_ads;

  for (const auto& ad : ads) {
    const double rand = base::RandDouble();
    if (rand <= ad.ptr)
      paced_ads.emplace_back(ad);
  }

  BLOG(2, paced_ads.size() << " eligible ads after pacing");

  return paced_ads;
}

}  // namespace ads
