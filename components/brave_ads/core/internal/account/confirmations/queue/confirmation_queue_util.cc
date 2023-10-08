/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/account/confirmations/queue/confirmation_queue_util.h"

#include <string>
#include <utility>

#include "base/functional/callback.h"
#include "brave/components/brave_ads/core/internal/account/confirmations/confirmation_info.h"
#include "brave/components/brave_ads/core/internal/account/confirmations/confirmations_util.h"
#include "brave/components/brave_ads/core/internal/account/confirmations/reward/reward_confirmation_util.h"
#include "brave/components/brave_ads/core/internal/account/confirmations/user_data_builder/dynamic/confirmation_dynamic_user_data_builder.h"
#include "brave/components/brave_ads/core/internal/common/logging_util.h"
#include "brave/components/brave_ads/core/internal/deprecated/confirmations/confirmation_state_manager.h"

namespace brave_ads {

namespace {

void RebuildConfirmationCallback(const ConfirmationInfo& confirmation,
                                 RebuildConfirmationQueueItemCallback callback,
                                 base::Value::Dict user_data) {
  ConfirmationInfo mutable_confirmation(confirmation);

  mutable_confirmation.user_data.dynamic = std::move(user_data);

  if (mutable_confirmation.reward) {
    const absl::optional<std::string> reward_credential_base64url =
        BuildRewardCredential(mutable_confirmation);
    CHECK(reward_credential_base64url);

    mutable_confirmation.reward->credential_base64url =
        *reward_credential_base64url;
  }

  std::move(callback).Run(mutable_confirmation);
}

}  // namespace

void AddConfirmationQueueItem(const ConfirmationInfo& confirmation) {
  ConfirmationStateManager::GetInstance().AddConfirmation(confirmation);
  ConfirmationStateManager::GetInstance().SaveState();
}

void RemoveConfirmationQueueItem(const ConfirmationInfo& confirmation) {
  if (!ConfirmationStateManager::GetInstance().RemoveConfirmation(
          confirmation)) {
    return BLOG(1,
                "Failed to remove confirmation queue item for transaction id "
                    << confirmation.transaction_id);
  }

  ConfirmationStateManager::GetInstance().SaveState();
}

absl::optional<ConfirmationInfo> MaybeGetNextConfirmationQueueItem() {
  const ConfirmationList confirmations =
      ConfirmationStateManager::GetInstance().GetConfirmations();
  if (confirmations.empty()) {
    return absl::nullopt;
  }

  return confirmations.front();
}

void RebuildConfirmationQueueItem(
    const ConfirmationInfo& confirmation,
    RebuildConfirmationQueueItemCallback callback) {
  CHECK(IsValid(confirmation));

  BuildDynamicUserData(base::BindOnce(&RebuildConfirmationCallback,
                                      confirmation, std::move(callback)));
}

}  // namespace brave_ads
