/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_CONVERSIONS_CONVERSION_QUEUE_ITEM_UNITTEST_UTIL_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_CONVERSIONS_CONVERSION_QUEUE_ITEM_UNITTEST_UTIL_H_

#include <string>

#include "brave/components/brave_ads/core/internal/conversions/conversion_queue_item_info.h"

namespace brave_ads {

class AdType;

void SaveConversionQueueItems(
    const ConversionQueueItemList& conversion_queue_items);

ConversionQueueItemInfo BuildConversionQueueItem(
    const AdType& ad_type,
    const std::string& conversion_id,
    const std::string& advertiser_public_key,
    bool should_use_random_uuids);

ConversionQueueItemList BuildAndSaveConversionQueueItems(
    const AdType& ad_type,
    const std::string& conversion_id,
    const std::string& advertiser_public_key,
    bool should_use_random_uuids,
    size_t count);

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_CONVERSIONS_CONVERSION_QUEUE_ITEM_UNITTEST_UTIL_H_
