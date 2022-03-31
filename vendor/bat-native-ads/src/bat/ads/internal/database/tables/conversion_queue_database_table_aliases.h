/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_DATABASE_TABLES_CONVERSION_QUEUE_DATABASE_TABLE_ALIASES_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_DATABASE_TABLES_CONVERSION_QUEUE_DATABASE_TABLE_ALIASES_H_

#include <functional>
#include <string>

#include "bat/ads/internal/conversions/conversion_queue_item_info_aliases.h"

namespace ads {

using GetConversionQueueCallback =
    std::function<void(const bool, const ConversionQueueItemList&)>;

using GetConversionQueueForCreativeInstanceIdCallback =
    std::function<void(const bool,
                       const std::string& creative_instance_id,
                       const ConversionQueueItemList&)>;

}  // namespace ads

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_DATABASE_TABLES_CONVERSION_QUEUE_DATABASE_TABLE_ALIASES_H_
