/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_CONVERSIONS_CONVERSION_QUEUE_ITEM_UNITTEST_UTIL_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_CONVERSIONS_CONVERSION_QUEUE_ITEM_UNITTEST_UTIL_H_

#include <string>

#include "bat/ads/internal/conversions/conversion_queue_item_info_aliases.h"

namespace ads {

void SaveConversionQueueItems(
    const ConversionQueueItemList& conversion_queue_items);

ConversionQueueItemInfo BuildConversionQueueItem(
    const std::string& conversion_id,
    const std::string& advertiser_public_key);

void BuildAndSaveConversionQueueItem(const std::string& conversion_id,
                                     const std::string& advertiser_public_key);

}  // namespace ads

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_CONVERSIONS_CONVERSION_QUEUE_ITEM_UNITTEST_UTIL_H_
