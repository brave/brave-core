/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/deprecated/user_engagement/conversions/queue/queue_item/conversion_queue_item_util.h"

#include <optional>

#include "base/time/time.h"
#include "brave/components/brave_ads/core/internal/deprecated/user_engagement/conversions/queue/queue_item/conversion_queue_item_info.h"
#include "brave/components/brave_ads/core/internal/deprecated/user_engagement/conversions/queue/queue_item/conversion_queue_item_util_constants.h"

namespace brave_ads {

namespace {

std::optional<base::TimeDelta>
    g_scoped_delay_before_processing_conversion_queue_item_for_testing;

base::TimeDelta DelayBeforeProcessingQueueItem(
    const ConversionQueueItemInfo& conversion_queue_item,
    const base::Time time) {
  return conversion_queue_item.process_at - time;
}

bool ShouldHaveProcessedQueueItemInThePast(
    const ConversionQueueItemInfo& conversion_queue_item,
    const base::Time time) {
  return DelayBeforeProcessingQueueItem(conversion_queue_item, time)
      .is_negative();
}

bool ShouldProcessQueueItem(
    const ConversionQueueItemInfo& conversion_queue_item,
    const base::Time time) {
  return time >= conversion_queue_item.process_at;
}

}  // namespace

base::TimeDelta CalculateDelayBeforeProcessingConversionQueueItem(
    const ConversionQueueItemInfo& conversion_queue_item) {
  if (g_scoped_delay_before_processing_conversion_queue_item_for_testing) {
    return *g_scoped_delay_before_processing_conversion_queue_item_for_testing;
  }

  const base::Time now = base::Time::Now();

  if (ShouldHaveProcessedQueueItemInThePast(conversion_queue_item, now) ||
      ShouldProcessQueueItem(conversion_queue_item, now)) {
    return kMinimumDelayBeforeProcessingConversionQueueItem;
  }

  const base::TimeDelta delay =
      DelayBeforeProcessingQueueItem(conversion_queue_item, now);
  if (delay < kMinimumDelayBeforeProcessingConversionQueueItem) {
    return kMinimumDelayBeforeProcessingConversionQueueItem;
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
      std::nullopt;
}

}  // namespace brave_ads
