/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BAT_ADS_INTERNAL_MINIMUM_WAIT_TIME_FREQUENCY_CAP_H_
#define BAT_ADS_INTERNAL_MINIMUM_WAIT_TIME_FREQUENCY_CAP_H_

#include <string>
#include <deque>

#include "bat/ads/internal/frequency_capping/permission_rule.h"

namespace ads {

struct CreativeAdNotificationInfo;
class AdsImpl;
class AdsClient;
class FrequencyCapping;

class MinimumWaitTimeFrequencyCap : public PermissionRule {
 public:
  MinimumWaitTimeFrequencyCap(
      const AdsImpl* const ads,
      const AdsClient* const ads_client,
      const FrequencyCapping* const frequency_capping);

  ~MinimumWaitTimeFrequencyCap() override;

  bool IsAllowed() override;

  std::string GetLastMessage() const override;

 private:
  const AdsImpl* const ads_;  // NOT OWNED
  const AdsClient* const ads_client_;  // NOT OWNED
  const FrequencyCapping* const frequency_capping_;  // NOT OWNED

  std::string last_message_;

  bool AreAdsAllowedAfterMinimumWaitTime(
      const std::deque<uint64_t>& history) const;
};

}  // namespace ads

#endif  // BAT_ADS_INTERNAL_MINIMUM_WAIT_TIME_FREQUENCY_CAP_H_
