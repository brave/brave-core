/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ACCOUNT_CONFIRMATIONS_QUEUE_QUEUE_ITEM_CONFIRMATION_QUEUE_ITEM_BUILDER_UTIL_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ACCOUNT_CONFIRMATIONS_QUEUE_QUEUE_ITEM_CONFIRMATION_QUEUE_ITEM_BUILDER_UTIL_H_

#include "brave/components/brave_ads/core/public/account/confirmations/confirmation_type.h"

namespace base {
class Time;
class TimeDelta;
}  // namespace base

namespace brave_ads {

// Returns the time at which the confirmation should be processed.
base::Time ProcessConfirmationAt(ConfirmationType confirmation_type);

// Returns the time at which the confirmation should be retried.
base::TimeDelta RetryProcessingConfirmationAfter();

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ACCOUNT_CONFIRMATIONS_QUEUE_QUEUE_ITEM_CONFIRMATION_QUEUE_ITEM_BUILDER_UTIL_H_
