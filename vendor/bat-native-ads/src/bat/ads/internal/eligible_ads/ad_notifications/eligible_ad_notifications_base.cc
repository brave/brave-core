/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/eligible_ads/ad_notifications/eligible_ad_notifications_base.h"

#include "bat/ads/internal/ad_serving/ad_targeting/geographic/subdivision/subdivision_targeting.h"
#include "bat/ads/internal/resources/frequency_capping/anti_targeting_resource.h"

namespace ads {
namespace ad_notifications {

EligibleAdsBase::EligibleAdsBase(
    ad_targeting::geographic::SubdivisionTargeting* subdivision_targeting,
    resource::AntiTargeting* anti_targeting_resource)
    : subdivision_targeting_(subdivision_targeting),
      anti_targeting_resource_(anti_targeting_resource) {
  DCHECK(subdivision_targeting_);
  DCHECK(anti_targeting_resource_);
}

EligibleAdsBase::~EligibleAdsBase() = default;

}  // namespace ad_notifications
}  // namespace ads
