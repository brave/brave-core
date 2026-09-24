/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ACCOUNT_CONFIRMATIONS_QUEUE_QUEUE_ITEM_CONFIRMATION_QUEUE_ITEM_INFO_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ACCOUNT_CONFIRMATIONS_QUEUE_QUEUE_ITEM_CONFIRMATION_QUEUE_ITEM_INFO_H_

#include <optional>
#include <vector>

#include "base/time/time.h"
#include "brave/components/brave_ads/core/internal/account/confirmations/confirmation_info.h"

namespace brave_ads {

struct ConfirmationQueueItemInfo final {
  ConfirmationQueueItemInfo();

  ConfirmationQueueItemInfo(const ConfirmationQueueItemInfo&);
  ConfirmationQueueItemInfo& operator=(const ConfirmationQueueItemInfo&);

  ConfirmationQueueItemInfo(ConfirmationQueueItemInfo&&) noexcept;
  ConfirmationQueueItemInfo& operator=(ConfirmationQueueItemInfo&&) noexcept;

  ~ConfirmationQueueItemInfo();

  bool operator==(const ConfirmationQueueItemInfo&) const = default;

  [[nodiscard]] bool IsValid() const;

  ConfirmationInfo confirmation;
  std::optional<base::Time> process_at;
  int retry_count = 0;
};

using ConfirmationQueueItemList = std::vector<ConfirmationQueueItemInfo>;

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ACCOUNT_CONFIRMATIONS_QUEUE_QUEUE_ITEM_CONFIRMATION_QUEUE_ITEM_INFO_H_
