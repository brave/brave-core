/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/account/confirmations/queue/queue_item/confirmation_queue_item_test_util.h"

#include "base/functional/bind.h"
#include "brave/components/brave_ads/core/internal/account/confirmations/queue/confirmation_queue_database_table.h"
#include "brave/components/brave_ads/core/internal/account/confirmations/queue/queue_item/confirmation_queue_item_builder.h"
#include "brave/components/brave_ads/core/internal/common/test/time_test_util.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace brave_ads::test {

ConfirmationQueueItemList BuildConfirmationQueueItems(
    const ConfirmationInfo& confirmation,
    const size_t count) {
  ConfirmationQueueItemList confirmation_queue_items;

  for (size_t i = 0; i < count; ++i) {
    const ConfirmationQueueItemInfo confirmation_queue_item =
        BuildConfirmationQueueItem(confirmation, /*process_at=*/Now());

    confirmation_queue_items.push_back(confirmation_queue_item);
  }

  return confirmation_queue_items;
}

void SaveConfirmationQueueItems(
    const ConfirmationQueueItemList& confirmation_queue_items) {
  const database::table::ConfirmationQueue database_table;
  database_table.Save(
      confirmation_queue_items,
      base::BindOnce([](const bool success) { ASSERT_TRUE(success); }));
}

void BuildAndSaveConfirmationQueueItems(const ConfirmationInfo& confirmation,
                                        const int count) {
  const ConfirmationQueueItemList confirmation_queue_items =
      BuildConfirmationQueueItems(confirmation, count);

  SaveConfirmationQueueItems(confirmation_queue_items);
}

}  // namespace brave_ads::test
