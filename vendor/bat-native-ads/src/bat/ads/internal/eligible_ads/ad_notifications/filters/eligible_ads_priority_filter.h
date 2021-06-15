/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_ELIGIBLE_ADS_AD_NOTIFICATIONS_FILTERS_ELIGIBLE_ADS_PRIORITY_FILTER_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_ELIGIBLE_ADS_AD_NOTIFICATIONS_FILTERS_ELIGIBLE_ADS_PRIORITY_FILTER_H_

#include "bat/ads/internal/eligible_ads/ad_notifications/filters/eligible_ads_filter.h"

namespace ads {

class EligibleAdsPriorityFilter : public EligibleAdsFilter {
 public:
  EligibleAdsPriorityFilter();
  ~EligibleAdsPriorityFilter() override;

  CreativeAdNotificationList Apply(
      const CreativeAdNotificationList& ads) const override;
};

}  // namespace ads

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_ELIGIBLE_ADS_AD_NOTIFICATIONS_FILTERS_ELIGIBLE_ADS_PRIORITY_FILTER_H_
