/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BAT_ADS_INTERNAL_ELIGIBLE_ADS_ELIGIBLE_ADS_PACING_FILTER_H_
#define BAT_ADS_INTERNAL_ELIGIBLE_ADS_ELIGIBLE_ADS_PACING_FILTER_H_

#include "bat/ads/internal/eligible_ads/eligible_ads_filter.h"

namespace ads {

class EligibleAdsPacingFilter : public EligibleAdsFilter {
 public:
  EligibleAdsPacingFilter();
  ~EligibleAdsPacingFilter() override;

  CreativeAdNotificationList Apply(
      const CreativeAdNotificationList& ads) const override;
};

}  // namespace ads

#endif  // BAT_ADS_INTERNAL_ELIGIBLE_ADS_ELIGIBLE_ADS_PACING_FILTER_H_
