/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ACCOUNT_CONFIRMATIONS_QUEUE_QUEUE_ITEM_CONFIRMATION_QUEUE_ITEM_UTIL_CONSTANTS_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ACCOUNT_CONFIRMATIONS_QUEUE_QUEUE_ITEM_CONFIRMATION_QUEUE_ITEM_UTIL_CONSTANTS_H_

#include "base/time/time.h"

namespace brave_ads {

inline constexpr base::TimeDelta
    kMinimumDelayBeforeProcessingConfirmationQueueItem = base::Seconds(5);

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ACCOUNT_CONFIRMATIONS_QUEUE_QUEUE_ITEM_CONFIRMATION_QUEUE_ITEM_UTIL_CONSTANTS_H_
