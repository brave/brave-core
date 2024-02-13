/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ACCOUNT_CONFIRMATIONS_QUEUE_CONFIRMATION_QUEUE_UTIL_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ACCOUNT_CONFIRMATIONS_QUEUE_CONFIRMATION_QUEUE_UTIL_H_

#include <optional>

#include "base/functional/callback.h"

namespace brave_ads {

struct ConfirmationInfo;

using RebuildConfirmationQueueItemCallback =
    base::OnceCallback<void(const ConfirmationInfo& confirmation)>;

void AddConfirmationQueueItem(const ConfirmationInfo& confirmation);
void RemoveConfirmationQueueItem(const ConfirmationInfo& confirmation);

std::optional<ConfirmationInfo> MaybeGetNextConfirmationQueueItem();

void RebuildConfirmationQueueItem(
    const ConfirmationInfo& confirmation,
    RebuildConfirmationQueueItemCallback callback);

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ACCOUNT_CONFIRMATIONS_QUEUE_CONFIRMATION_QUEUE_UTIL_H_
