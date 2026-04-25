/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/account/confirmations/queue/queue_item/confirmation_queue_item_info.h"

namespace brave_ads {

ConfirmationQueueItemInfo::ConfirmationQueueItemInfo() = default;

ConfirmationQueueItemInfo::ConfirmationQueueItemInfo(
    const ConfirmationQueueItemInfo& other) = default;

ConfirmationQueueItemInfo& ConfirmationQueueItemInfo::operator=(
    const ConfirmationQueueItemInfo& other) = default;

ConfirmationQueueItemInfo::ConfirmationQueueItemInfo(
    ConfirmationQueueItemInfo&& other) noexcept = default;

ConfirmationQueueItemInfo& ConfirmationQueueItemInfo::operator=(
    ConfirmationQueueItemInfo&& other) noexcept = default;

ConfirmationQueueItemInfo::~ConfirmationQueueItemInfo() = default;

bool ConfirmationQueueItemInfo::IsValid() const {
  return !!process_at;
}

}  // namespace brave_ads
