/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/conversions/queue/queue_item/conversion_queue_item_unittest_util.h"

#include "base/check.h"
#include "base/functional/bind.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_time_util.h"
#include "brave/components/brave_ads/core/internal/conversions/queue/conversion_queue_database_table.h"
#include "brave/components/brave_ads/core/internal/conversions/queue/queue_item/conversion_queue_item_builder.h"

namespace brave_ads {

ConversionQueueItemList BuildConversionQueueItems(
    const ConversionInfo& conversion,
    const size_t count) {
  ConversionQueueItemList conversion_queue_items;

  for (size_t i = 0; i < count; i++) {
    const ConversionQueueItemInfo conversion_queue_item =
        BuildConversionQueueItem(conversion, /*process_at*/ Now());

    conversion_queue_items.push_back(conversion_queue_item);
  }

  return conversion_queue_items;
}

void SaveConversionQueueItems(
    const ConversionQueueItemList& conversion_queue_items) {
  const database::table::ConversionQueue database_table;
  database_table.Save(
      conversion_queue_items,
      base::BindOnce([](const bool success) { CHECK(success); }));
}

}  // namespace brave_ads
