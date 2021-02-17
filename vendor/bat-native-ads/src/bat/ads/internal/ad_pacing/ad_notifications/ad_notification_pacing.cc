/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/ad_pacing/ad_notifications/ad_notification_pacing.h"

#include "base/rand_util.h"
#include "bat/ads/internal/eligible_ads/ad_notifications/filters/eligible_ads_filter_factory.h"
#include "bat/ads/internal/logging.h"

namespace ads {
namespace ad_notifications {

AdPacing::AdPacing() = default;

AdPacing::~AdPacing() = default;

CreativeAdNotificationList AdPacing::PaceAds(
    const CreativeAdNotificationList& ads) {
  CreativeAdNotificationList paced_ads = ads;

  const auto iter = std::remove_if(
      paced_ads.begin(), paced_ads.end(),
      [&](const CreativeAdNotificationInfo& ad) { return ShouldPace(ad); });

  paced_ads.erase(iter, paced_ads.end());

  const auto priority_filter =
      EligibleAdsFilterFactory::Build(EligibleAdsFilter::Type::kPriority);
  DCHECK(priority_filter);
  return priority_filter->Apply(paced_ads);
}

///////////////////////////////////////////////////////////////////////////////

bool AdPacing::ShouldPace(const CreativeAdNotificationInfo& ad) {
  const double rand = base::RandDouble();
  if (rand <= ad.ptr) {
    return false;
  }

  BLOG(2, "Pacing ad delivery for creative instance id "
              << ad.creative_instance_id << " [Roll(" << ad.ptr << "):" << rand
              << "]");

  return true;
}

}  // namespace ad_notifications
}  // namespace ads
