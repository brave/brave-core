/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/account/confirmations/confirmations.h"

#include <utility>

#include "absl/types/optional.h"
#include "base/functional/bind.h"
#include "base/time/time.h"
#include "bat/ads/internal/account/confirmations/confirmation_user_data_builder.h"
#include "bat/ads/internal/account/confirmations/confirmation_util.h"
#include "bat/ads/internal/account/transactions/transaction_info.h"
#include "bat/ads/internal/account/utility/redeem_unblinded_token/redeem_unblinded_token.h"
#include "bat/ads/internal/ads_client_helper.h"
#include "bat/ads/internal/common/logging_util.h"
#include "bat/ads/internal/common/time/time_formatting_util.h"
#include "bat/ads/internal/deprecated/confirmations/confirmation_state_manager.h"
#include "bat/ads/internal/privacy/tokens/token_generator_interface.h"
#include "bat/ads/internal/privacy/tokens/unblinded_payment_tokens/unblinded_payment_token_info.h"
#include "bat/ads/internal/privacy/tokens/unblinded_payment_tokens/unblinded_payment_token_util.h"
#include "brave/components/brave_ads/common/pref_names.h"

namespace ads {

namespace {

constexpr base::TimeDelta kRetryAfter = base::Seconds(15);

void AppendToRetryQueue(const ConfirmationInfo& confirmation) {
  DCHECK(IsValid(confirmation));

  ConfirmationStateManager::GetInstance()->AppendFailedConfirmation(
      confirmation);
  ConfirmationStateManager::GetInstance()->Save();

  BLOG(1, "Added " << confirmation.type << " confirmation for "
                   << confirmation.ad_type << " with transaction id "
                   << confirmation.transaction_id
                   << " and creative instance id "
                   << confirmation.creative_instance_id
                   << " to the confirmations queue");
}

void RemoveFromRetryQueue(const ConfirmationInfo& confirmation) {
  DCHECK(IsValid(confirmation));

  if (!ConfirmationStateManager::GetInstance()->RemoveFailedConfirmation(
          confirmation)) {
    BLOG(0, "Failed to remove "
                << confirmation.type << " confirmation for "
                << confirmation.ad_type << " with transaction id "
                << confirmation.transaction_id << " and creative instance id "
                << confirmation.creative_instance_id
                << " from the confirmations queue");

    return;
  }

  BLOG(1, "Removed " << confirmation.type << " confirmation for "
                     << confirmation.ad_type << " with transaction id "
                     << confirmation.transaction_id
                     << " and creative instance id "
                     << confirmation.creative_instance_id
                     << " from the confirmations queue");

  ConfirmationStateManager::GetInstance()->Save();
}

}  // namespace

Confirmations::Confirmations(privacy::TokenGeneratorInterface* token_generator)
    : token_generator_(token_generator),
      redeem_unblinded_token_(std::make_unique<RedeemUnblindedToken>()) {
  DCHECK(token_generator_);

  redeem_unblinded_token_->SetDelegate(this);
}

Confirmations::~Confirmations() {
  delegate_ = nullptr;
}

void Confirmations::Confirm(const TransactionInfo& transaction) {
  DCHECK(transaction.IsValid());

  BLOG(1, "Confirming " << transaction.confirmation_type << " for "
                        << transaction.ad_type << " with transaction id "
                        << transaction.id << " and creative instance id "
                        << transaction.creative_instance_id);

  const base::Time created_at = base::Time::Now();

  const ConfirmationUserDataBuilder user_data_builder(
      created_at, transaction.creative_instance_id,
      transaction.confirmation_type);
  user_data_builder.Build(
      base::BindOnce(&Confirmations::CreateConfirmationAndRedeemToken,
                     base::Unretained(this), transaction, created_at));
}

void Confirmations::ProcessRetryQueue() {
  if (retry_timer_.IsRunning()) {
    return;
  }

  Retry();
}

///////////////////////////////////////////////////////////////////////////////

void Confirmations::Retry() {
  const ConfirmationList& failed_confirmations =
      ConfirmationStateManager::GetInstance()->GetFailedConfirmations();
  if (failed_confirmations.empty()) {
    BLOG(1, "No failed confirmations to retry");
    return;
  }

  DCHECK(!retry_timer_.IsRunning());
  const base::Time retry_at = retry_timer_.StartWithPrivacy(
      FROM_HERE, kRetryAfter,
      base::BindOnce(&Confirmations::OnRetry, base::Unretained(this)));

  BLOG(1, "Retry sending failed confirmations "
              << FriendlyDateAndTime(retry_at, /*use_sentence_style*/ true));
}

void Confirmations::OnRetry() {
  const ConfirmationList& failed_confirmations =
      ConfirmationStateManager::GetInstance()->GetFailedConfirmations();
  DCHECK(!failed_confirmations.empty());

  BLOG(1, "Retry sending failed confirmations");

  const ConfirmationInfo failed_confirmation_copy =
      failed_confirmations.front();
  RemoveFromRetryQueue(failed_confirmation_copy);

  redeem_unblinded_token_->Redeem(failed_confirmation_copy);
}

void Confirmations::StopRetrying() {
  retry_timer_.Stop();
}

void Confirmations::CreateConfirmationAndRedeemToken(
    const TransactionInfo& transaction,
    const base::Time& created_at,
    base::Value::Dict user_data) {
  const absl::optional<ConfirmationInfo> confirmation = CreateConfirmation(
      token_generator_, created_at, transaction.id,
      transaction.creative_instance_id, transaction.confirmation_type,
      transaction.ad_type, std::move(user_data));
  if (!confirmation) {
    BLOG(0, "Failed to confirm confirmation");
    return;
  }

  redeem_unblinded_token_->Redeem(*confirmation);
}

void Confirmations::CreateNewConfirmationAndAppendToRetryQueue(
    const ConfirmationInfo& confirmation,
    base::Value::Dict user_data) {
  const absl::optional<ConfirmationInfo> new_confirmation = CreateConfirmation(
      token_generator_, confirmation.created_at, confirmation.transaction_id,
      confirmation.creative_instance_id, confirmation.type,
      confirmation.ad_type, std::move(user_data));
  if (!new_confirmation) {
    AppendToRetryQueue(confirmation);
    return;
  }

  AppendToRetryQueue(*new_confirmation);
}

void Confirmations::OnDidSendConfirmation(
    const ConfirmationInfo& confirmation) {
  if (delegate_) {
    delegate_->OnDidConfirm(confirmation);
  }

  StopRetrying();

  ProcessRetryQueue();
}

void Confirmations::OnFailedToSendConfirmation(
    const ConfirmationInfo& confirmation,
    const bool should_retry) {
  if (should_retry) {
    AppendToRetryQueue(confirmation);
  }

  if (delegate_) {
    delegate_->OnFailedToConfirm(confirmation);
  }

  ProcessRetryQueue();
}

void Confirmations::OnDidRedeemUnblindedToken(
    const ConfirmationInfo& confirmation,
    const privacy::UnblindedPaymentTokenInfo& unblinded_payment_token) {
  if (privacy::UnblindedPaymentTokenExists(unblinded_payment_token)) {
    BLOG(1, "Unblinded payment token is a duplicate");
    OnFailedToRedeemUnblindedToken(confirmation, /*should_retry*/ false,
                                   /*should_backoff*/ false);
    return;
  }

  privacy::AddUnblindedPaymentTokens({unblinded_payment_token});

  const base::Time next_token_redemption_at =
      AdsClientHelper::GetInstance()->GetTimePref(
          prefs::kNextTokenRedemptionAt);

  BLOG(1, "You have " << privacy::UnblindedPaymentTokenCount()
                      << " unblinded payment tokens which will be redeemed "
                      << FriendlyDateAndTime(next_token_redemption_at,
                                             /*use_sentence_style*/ true));

  if (delegate_) {
    delegate_->OnDidConfirm(confirmation);
  }

  StopRetrying();

  ProcessRetryQueue();
}

void Confirmations::OnFailedToRedeemUnblindedToken(
    const ConfirmationInfo& confirmation,
    const bool should_retry,
    const bool should_backoff) {
  DCHECK(IsValid(confirmation));

  if (should_retry) {
    if (!confirmation.was_created) {
      const ConfirmationUserDataBuilder user_data_builder(
          confirmation.created_at, confirmation.creative_instance_id,
          confirmation.type);
      user_data_builder.Build(base::BindOnce(
          &Confirmations::CreateNewConfirmationAndAppendToRetryQueue,
          base::Unretained(this), confirmation));
    } else {
      AppendToRetryQueue(confirmation);
    }
  }

  if (delegate_) {
    delegate_->OnFailedToConfirm(confirmation);
  }

  if (!should_backoff) {
    StopRetrying();
  }

  ProcessRetryQueue();
}

}  // namespace ads
