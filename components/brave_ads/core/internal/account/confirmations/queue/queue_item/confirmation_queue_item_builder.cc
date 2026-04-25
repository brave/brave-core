/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/account/confirmations/queue/queue_item/confirmation_queue_item_builder.h"

#include "base/time/time.h"
#include "brave/components/brave_ads/core/internal/account/confirmations/queue/queue_item/confirmation_queue_item_info.h"

namespace brave_ads {

ConfirmationQueueItemInfo BuildConfirmationQueueItem(
    const ConfirmationInfo& confirmation,
    base::Time process_at) {
  ConfirmationQueueItemInfo confirmation_queue_item;
  confirmation_queue_item.confirmation = confirmation;
  confirmation_queue_item.process_at = process_at;

  return confirmation_queue_item;
}

}  // namespace brave_ads
