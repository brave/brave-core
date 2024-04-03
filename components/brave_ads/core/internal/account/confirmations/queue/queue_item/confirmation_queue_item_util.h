/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ACCOUNT_CONFIRMATIONS_QUEUE_QUEUE_ITEM_CONFIRMATION_QUEUE_ITEM_UTIL_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ACCOUNT_CONFIRMATIONS_QUEUE_QUEUE_ITEM_CONFIRMATION_QUEUE_ITEM_UTIL_H_

namespace base {
class TimeDelta;
}  // namespace base

namespace brave_ads {

struct ConfirmationInfo;
struct ConfirmationQueueItemInfo;

base::TimeDelta CalculateDelayBeforeProcessingConfirmationQueueItem(
    const ConfirmationQueueItemInfo& confirmation_queue_item);

class ScopedDelayBeforeProcessingConfirmationQueueItemForTesting final {
 public:
  explicit ScopedDelayBeforeProcessingConfirmationQueueItemForTesting(
      base::TimeDelta delay);

  ~ScopedDelayBeforeProcessingConfirmationQueueItemForTesting();
};

ConfirmationInfo RebuildConfirmationWithoutDynamicUserData(
    const ConfirmationInfo& confirmation);

ConfirmationInfo RebuildConfirmationDynamicUserData(
    const ConfirmationInfo& confirmation);

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ACCOUNT_CONFIRMATIONS_QUEUE_QUEUE_ITEM_CONFIRMATION_QUEUE_ITEM_UTIL_H_
