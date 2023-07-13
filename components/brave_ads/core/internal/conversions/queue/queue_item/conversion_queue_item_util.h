/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_CONVERSIONS_QUEUE_QUEUE_ITEM_CONVERSION_QUEUE_ITEM_UTIL_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_CONVERSIONS_QUEUE_QUEUE_ITEM_CONVERSION_QUEUE_ITEM_UTIL_H_

namespace base {
class TimeDelta;
}  // namespace base

namespace brave_ads {

struct ConversionQueueItemInfo;

base::TimeDelta CalculateDelayBeforeProcessingConversionQueueItem(
    const ConversionQueueItemInfo& conversion_queue_item);

class ScopedDelayBeforeProcessingConversionQueueItemForTesting final {
 public:
  explicit ScopedDelayBeforeProcessingConversionQueueItemForTesting(
      base::TimeDelta delay);

  ~ScopedDelayBeforeProcessingConversionQueueItemForTesting();
};

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_CONVERSIONS_QUEUE_QUEUE_ITEM_CONVERSION_QUEUE_ITEM_UTIL_H_
