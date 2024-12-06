/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/account/confirmations/confirmations.h"

#include <optional>
#include <utility>

#include "base/time/time.h"
#include "brave/components/brave_ads/core/internal/account/confirmations/confirmation_info.h"
#include "brave/components/brave_ads/core/internal/account/confirmations/non_reward/non_reward_confirmation_util.h"
#include "brave/components/brave_ads/core/internal/account/confirmations/queue/confirmation_queue.h"
#include "brave/components/brave_ads/core/internal/account/confirmations/reward/reward_confirmation_util.h"
#include "brave/components/brave_ads/core/internal/account/confirmations/user_data_builder/confirmation_user_data_builder.h"
#include "brave/components/brave_ads/core/internal/account/transactions/transaction_info.h"
#include "brave/components/brave_ads/core/internal/common/logging_util.h"
#include "brave/components/brave_ads/core/internal/common/time/time_formatting_util.h"
#include "brave/components/brave_ads/core/internal/settings/settings.h"

namespace brave_ads {

Confirmations::Confirmations() {
  confirmation_queue_.SetDelegate(this);
}

Confirmations::~Confirmations() {
  delegate_ = nullptr;
}

void Confirmations::Confirm(const TransactionInfo& transaction,
                            base::Value::Dict user_data) {
  CHECK(transaction.IsValid());

  BLOG(1, "Confirming " << transaction.confirmation_type << " for "
                        << transaction.ad_type << " with transaction id "
                        << transaction.id << " and creative instance id "
                        << transaction.creative_instance_id);

  const std::optional<ConfirmationInfo> confirmation =
      UserHasJoinedBraveRewards()
          ? BuildRewardConfirmation(transaction, std::move(user_data))
          : BuildNonRewardConfirmation(transaction, std::move(user_data));
  if (!confirmation) {
    return BLOG(0, "Failed to build confirmation");
  }

  confirmation_queue_.Add(*confirmation);
}

void Confirmations::NotifyDidConfirm(
    const ConfirmationInfo& confirmation) const {
  if (delegate_) {
    delegate_->OnDidConfirm(confirmation);
  }
}

void Confirmations::NotifyFailedToConfirm(
    const ConfirmationInfo& confirmation) const {
  if (delegate_) {
    delegate_->OnFailedToConfirm(confirmation);
  }
}

void Confirmations::OnDidAddConfirmationToQueue(
    const ConfirmationInfo& confirmation) {
  BLOG(1, "Successfully added "
              << confirmation.type << " confirmation for "
              << confirmation.ad_type << " with transaction id "
              << confirmation.transaction_id << " and creative instance id "
              << confirmation.creative_instance_id << " to the queue");
}

void Confirmations::OnWillProcessConfirmationQueue(
    const ConfirmationInfo& confirmation,
    base::Time process_at) {
  BLOG(1, "Process " << confirmation.type << " confirmation for "
                     << confirmation.ad_type << " with transaction id "
                     << confirmation.transaction_id
                     << " and creative instance id "
                     << confirmation.creative_instance_id << " "
                     << FriendlyDateAndTime(process_at));
}

void Confirmations::OnDidProcessConfirmationQueue(
    const ConfirmationInfo& confirmation) {
  BLOG(1, "Successfully processed "
              << confirmation.type << " confirmation for "
              << confirmation.ad_type << " with transaction id "
              << confirmation.transaction_id << " and creative instance id "
              << confirmation.creative_instance_id);

  NotifyDidConfirm(confirmation);
}

void Confirmations::OnFailedToProcessConfirmationQueue(
    const ConfirmationInfo& confirmation) {
  BLOG(1, "Failed to process "
              << confirmation.type << " confirmation for "
              << confirmation.ad_type << " with transaction id "
              << confirmation.transaction_id << " and creative instance id "
              << confirmation.creative_instance_id);

  NotifyFailedToConfirm(confirmation);
}

void Confirmations::OnDidExhaustConfirmationQueue() {
  BLOG(1, "Confirmation queue is exhausted");
}

}  // namespace brave_ads
