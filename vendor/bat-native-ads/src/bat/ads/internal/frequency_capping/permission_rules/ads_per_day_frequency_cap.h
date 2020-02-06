/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BAT_ADS_INTERNAL_PER_DAY_LIMIT_H_
#define BAT_ADS_INTERNAL_PER_DAY_LIMIT_H_

#include <string>

#include "bat/ads/internal/frequency_capping/permission_rule.h"

namespace ads {

struct CreativeAdNotificationInfo;
class AdsClient;
class FrequencyCapping;

class AdsPerDayFrequencyCap : public PermissionRule  {
 public:
  AdsPerDayFrequencyCap(
      const AdsClient* const ads_client,
      const FrequencyCapping* const frequency_capping);

  ~AdsPerDayFrequencyCap() override;

  bool IsAllowed() override;

  const std::string GetLastMessage() const override;

 private:
  const AdsClient* const ads_client_;  // NOT OWNED
  const FrequencyCapping* const frequency_capping_;  // NOT OWNED

  std::string last_message_;

  bool AreAdsPerDayBelowAllowedThreshold() const;
};

}  // namespace ads

#endif  // BAT_ADS_INTERNAL_PER_DAY_LIMIT_H_
