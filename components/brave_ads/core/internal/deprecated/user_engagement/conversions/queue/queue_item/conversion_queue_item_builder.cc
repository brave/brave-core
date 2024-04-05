/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/deprecated/user_engagement/conversions/queue/queue_item/conversion_queue_item_builder.h"

#include "base/time/time.h"
#include "brave/components/brave_ads/core/internal/deprecated/user_engagement/conversions/queue/queue_item/conversion_queue_item_info.h"

namespace brave_ads {

ConversionQueueItemInfo BuildConversionQueueItem(
    const ConversionInfo& conversion,
    const base::Time process_at) {
  ConversionQueueItemInfo conversion_queue_item;
  conversion_queue_item.conversion = conversion;
  conversion_queue_item.process_at = process_at;

  return conversion_queue_item;
}

}  // namespace brave_ads
