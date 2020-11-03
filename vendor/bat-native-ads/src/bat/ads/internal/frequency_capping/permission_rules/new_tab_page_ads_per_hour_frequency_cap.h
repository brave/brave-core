/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BAT_ADS_INTERNAL_FREQUENCY_CAPPING_PERMISSION_RULES_NEW_TAB_PAGE_ADS_PER_HOUR_FREQUENCY_CAP_H_  // NOLINT
#define BAT_ADS_INTERNAL_FREQUENCY_CAPPING_PERMISSION_RULES_NEW_TAB_PAGE_ADS_PER_HOUR_FREQUENCY_CAP_H_  // NOLINT

#include <string>

#include "bat/ads/internal/ad_events/ad_event_info.h"
#include "bat/ads/internal/frequency_capping/permission_rules/permission_rule.h"

namespace ads {

class AdsImpl;

class NewTabPageAdsPerHourFrequencyCap : public PermissionRule {
 public:
  NewTabPageAdsPerHourFrequencyCap(
      AdsImpl* ads,
      const AdEventList& ad_events);

  ~NewTabPageAdsPerHourFrequencyCap() override;

  NewTabPageAdsPerHourFrequencyCap(
      const NewTabPageAdsPerHourFrequencyCap&) = delete;
  NewTabPageAdsPerHourFrequencyCap& operator=(
      const NewTabPageAdsPerHourFrequencyCap&) = delete;

  bool ShouldAllow() override;

  std::string get_last_message() const override;

 private:
  AdsImpl* ads_;  // NOT OWNED

  AdEventList ad_events_;

  std::string last_message_;

  bool DoesRespectCap(
      const AdEventList& ad_events);

  AdEventList FilterAdEvents(
      const AdEventList& ad_events) const;
};

}  // namespace ads

#endif  // BAT_ADS_INTERNAL_FREQUENCY_CAPPING_PERMISSION_RULES_NEW_TAB_PAGE_ADS_PER_HOUR_FREQUENCY_CAP_H_  // NOLINT
