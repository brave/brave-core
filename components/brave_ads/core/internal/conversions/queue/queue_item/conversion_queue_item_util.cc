/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/conversions/queue/queue_item/conversion_queue_item_util.h"

#include "base/time/time.h"
#include "brave/components/brave_ads/core/internal/conversions/queue/queue_item/conversion_queue_item_info.h"
#include "brave/components/brave_ads/core/internal/conversions/queue/queue_item/conversion_queue_item_util_constants.h"

namespace brave_ads {

namespace {

absl::optional<base::TimeDelta>
    g_scoped_delay_before_processing_conversion_queue_item_for_testing;

base::TimeDelta DelayBeforeProcessingQueueItem(
    const ConversionQueueItemInfo& conversion_queue_item) {
  return conversion_queue_item.process_at - base::Time::Now();
}

bool ShouldHaveProcessedConversionQueueItemInThePast(
    const ConversionQueueItemInfo& conversion_queue_item) {
  return DelayBeforeProcessingQueueItem(conversion_queue_item).is_negative();
}

bool ShouldProcessConversionQueueItem(
    const ConversionQueueItemInfo& conversion_queue_item) {
  return base::Time::Now() >= conversion_queue_item.process_at;
}

}  // namespace

base::TimeDelta CalculateDelayBeforeProcessingConversionQueueItem(
    const ConversionQueueItemInfo& conversion_queue_item) {
  if (g_scoped_delay_before_processing_conversion_queue_item_for_testing) {
    return *g_scoped_delay_before_processing_conversion_queue_item_for_testing;
  }

  if (ShouldHaveProcessedConversionQueueItemInThePast(conversion_queue_item) ||
      ShouldProcessConversionQueueItem(conversion_queue_item)) {
    return kMinimumDelayBeforeProcessingQueueItem;
  }

  const base::TimeDelta delay =
      DelayBeforeProcessingQueueItem(conversion_queue_item);
  if (delay < kMinimumDelayBeforeProcessingQueueItem) {
    return kMinimumDelayBeforeProcessingQueueItem;
  }

  return delay;
}

ScopedDelayBeforeProcessingConversionQueueItemForTesting::
    ScopedDelayBeforeProcessingConversionQueueItemForTesting(
        const base::TimeDelta delay) {
  g_scoped_delay_before_processing_conversion_queue_item_for_testing = delay;
}

ScopedDelayBeforeProcessingConversionQueueItemForTesting::
    ~ScopedDelayBeforeProcessingConversionQueueItemForTesting() {
  g_scoped_delay_before_processing_conversion_queue_item_for_testing =
      absl::nullopt;
}

}  // namespace brave_ads
