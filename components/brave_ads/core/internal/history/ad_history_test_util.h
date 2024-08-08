/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_HISTORY_AD_HISTORY_TEST_UTIL_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_HISTORY_AD_HISTORY_TEST_UTIL_H_

#include <vector>

#include "brave/components/brave_ads/core/public/account/confirmations/confirmation_type.h"
#include "brave/components/brave_ads/core/public/history/ad_history_item_info.h"

namespace brave_ads::test {

AdHistoryItemInfo BuildAdHistoryItem(AdType ad_type,
                                     ConfirmationType confirmation_type,
                                     bool should_generate_random_uuids);

AdHistoryList BuildAdHistory(
    AdType ad_type,
    const std::vector<ConfirmationType>& confirmation_types,
    bool should_generate_random_uuids);

AdHistoryList BuildAdHistoryForSamePlacement(
    AdType ad_type,
    const std::vector<ConfirmationType>& confirmation_types,
    bool should_generate_random_uuids);

}  // namespace brave_ads::test

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_HISTORY_AD_HISTORY_TEST_UTIL_H_
