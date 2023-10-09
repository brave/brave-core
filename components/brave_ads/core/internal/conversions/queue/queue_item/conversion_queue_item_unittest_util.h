/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_CONVERSIONS_QUEUE_QUEUE_ITEM_CONVERSION_QUEUE_ITEM_UNITTEST_UTIL_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_CONVERSIONS_QUEUE_QUEUE_ITEM_CONVERSION_QUEUE_ITEM_UNITTEST_UTIL_H_

#include "brave/components/brave_ads/core/internal/conversions/queue/queue_item/conversion_queue_item_info.h"
#include "brave/components/brave_ads/core/public/account/confirmations/confirmation_type.h"

namespace brave_ads {

class AdType;
class ConfirmationType;
struct ConversionInfo;

namespace test {

ConversionQueueItemList BuildConversionQueueItems(
    const ConversionInfo& conversion,
    size_t count);

void SaveConversionQueue(const ConversionQueueItemList& conversion_queue_items);

void BuildAndSaveConversionQueue(const AdType& ad_type,
                                 const ConfirmationType& confirmation_type,
                                 bool is_verifiable,
                                 bool should_use_random_uuids,
                                 int count);

}  // namespace test

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_CONVERSIONS_QUEUE_QUEUE_ITEM_CONVERSION_QUEUE_ITEM_UNITTEST_UTIL_H_
