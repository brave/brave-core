/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ACCOUNT_CONFIRMATIONS_QUEUE_CONFIRMATION_QUEUE_DATABASE_TABLE_UTIL_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ACCOUNT_CONFIRMATIONS_QUEUE_CONFIRMATION_QUEUE_DATABASE_TABLE_UTIL_H_

#include "brave/components/brave_ads/core/internal/account/confirmations/queue/queue_item/confirmation_queue_item_info.h"
#include "brave/components/brave_ads/core/mojom/brave_ads.mojom-forward.h"

namespace brave_ads::database::table {

// Converts a single database row to a `ConfirmationQueueItemInfo`, decoding
// base64 token fields and deserializing user_data JSON.
ConfirmationQueueItemInfo ConfirmationQueueItemFromMojomRow(
    const mojom::DBRowInfoPtr& mojom_db_row);

}  // namespace brave_ads::database::table

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ACCOUNT_CONFIRMATIONS_QUEUE_CONFIRMATION_QUEUE_DATABASE_TABLE_UTIL_H_
