/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_ELIGIBLE_ADS_ROUND_ROBIN_ADVERTISERS_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_ELIGIBLE_ADS_ROUND_ROBIN_ADVERTISERS_H_

#include <map>
#include <string>

#include "bat/ads/internal/bundle/creative_ad_info.h"
#include "bat/ads/internal/frequency_capping/exclusion_rules/split_test_frequency_cap.h"

namespace ads {

template <typename T>
T FilterSeenAdvertisers(const T& ads,
                        const std::map<std::string, bool>& seen_advertisers) {
  T unseen_advertisers = ads;

  const auto iter =
      std::remove_if(unseen_advertisers.begin(), unseen_advertisers.end(),
                     [&seen_advertisers](CreativeAdInfo& ad) {
                       SplitTestFrequencyCap frequency_cap;
                       return frequency_cap.ShouldExclude(ad) ||
                              seen_advertisers.find(ad.advertiser_id) !=
                                  seen_advertisers.end();
                     });

  unseen_advertisers.erase(iter, unseen_advertisers.end());

  return unseen_advertisers;
}

}  // namespace ads

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_ELIGIBLE_ADS_ROUND_ROBIN_ADVERTISERS_H_
