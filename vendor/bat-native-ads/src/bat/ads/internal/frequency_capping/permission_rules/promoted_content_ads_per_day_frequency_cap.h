/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BAT_ADS_INTERNAL_FREQUENCY_CAPPING_PERMISSION_RULES_PROMOTED_CONTENT_ADS_PER_DAY_FREQUENCY_CAP_H_  // NOLINT
#define BAT_ADS_INTERNAL_FREQUENCY_CAPPING_PERMISSION_RULES_PROMOTED_CONTENT_ADS_PER_DAY_FREQUENCY_CAP_H_  // NOLINT

#include <stdint.h>

#include <string>

#include "bat/ads/internal/ad_events/ad_event_info.h"
#include "bat/ads/internal/frequency_capping/permission_rules/permission_rule.h"

namespace ads {

const uint64_t kPromotedContentAdsPerDayFrequencyCap = 20;

class PromotedContentAdsPerDayFrequencyCap : public PermissionRule  {
 public:
  PromotedContentAdsPerDayFrequencyCap(
      const AdEventList& ad_events);

  ~PromotedContentAdsPerDayFrequencyCap() override;

  PromotedContentAdsPerDayFrequencyCap(
      const PromotedContentAdsPerDayFrequencyCap&) = delete;
  PromotedContentAdsPerDayFrequencyCap& operator=(
      const PromotedContentAdsPerDayFrequencyCap&) = delete;

  bool ShouldAllow() override;

  std::string get_last_message() const override;

 private:
  AdEventList ad_events_;

  std::string last_message_;

  bool DoesRespectCap(
      const AdEventList& ad_events);

  AdEventList FilterAdEvents(
      const AdEventList& ad_events) const;
};

}  // namespace ads

#endif  // BAT_ADS_INTERNAL_FREQUENCY_CAPPING_PERMISSION_RULES_PROMOTED_CONTENT_ADS_PER_DAY_FREQUENCY_CAP_H_  // NOLINT
