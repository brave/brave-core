/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/legacy_migration/confirmations/legacy_confirmation_migration_confirmation_queue_builder.h"

#include "base/time/time.h"
#include "brave/components/brave_ads/core/internal/account/confirmations/queue/queue_item/confirmation_queue_item_builder.h"

namespace brave_ads {

ConfirmationQueueItemList BuildConfirmationQueueItems(
    const std::optional<ConfirmationList>& confirmations) {
  if (!confirmations) {
    return {};
  }

  ConfirmationQueueItemList confirmation_queue_items;
  confirmation_queue_items.reserve(confirmations->size());
  for (const auto& confirmation : *confirmations) {
    confirmation_queue_items.push_back(BuildConfirmationQueueItem(
        confirmation, /*process_at=*/base::Time::Now()));
  }
  return confirmation_queue_items;
}

}  // namespace brave_ads
