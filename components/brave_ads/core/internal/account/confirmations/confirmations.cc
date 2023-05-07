/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/account/confirmations/confirmations.h"

#include <utility>

#include "base/functional/bind.h"
#include "base/time/time.h"
#include "brave/components/brave_ads/common/pref_names.h"
#include "brave/components/brave_ads/core/internal/account/confirmations/confirmation_dynamic_user_data_builder.h"
#include "brave/components/brave_ads/core/internal/account/confirmations/confirmation_user_data_builder.h"
#include "brave/components/brave_ads/core/internal/account/confirmations/confirmation_util.h"
#include "brave/components/brave_ads/core/internal/account/confirmations/opted_in_user_data_info.h"
#include "brave/components/brave_ads/core/internal/account/transactions/transaction_info.h"
#include "brave/components/brave_ads/core/internal/account/utility/redeem_confirmation/redeem_confirmation_factory.h"
#include "brave/components/brave_ads/core/internal/ads_client_helper.h"
#include "brave/components/brave_ads/core/internal/common/logging_util.h"
#include "brave/components/brave_ads/core/internal/common/time/time_formatting_util.h"
#include "brave/components/brave_ads/core/internal/deprecated/confirmations/confirmation_state_manager.h"
#include "brave/components/brave_ads/core/internal/privacy/tokens/token_generator_interface.h"
#include "brave/components/brave_ads/core/internal/privacy/tokens/unblinded_payment_tokens/unblinded_payment_token_info.h"
#include "brave/components/brave_ads/core/internal/privacy/tokens/unblinded_payment_tokens/unblinded_payment_token_util.h"
#include "third_party/abseil-cpp/absl/types/optional.h"

namespace brave_ads {

namespace {

constexpr base::TimeDelta kRetryAfter = base::Seconds(15);

void AppendToRetryQueue(const ConfirmationInfo& confirmation) {
  CHECK(IsValid(confirmation));

  ConfirmationStateManager::GetInstance().AppendFailedConfirmation(
      confirmation);
  ConfirmationStateManager::GetInstance().Save();

  BLOG(1, "Added " << confirmation.type << " confirmation for "
                   << confirmation.ad_type << " with transaction id "
                   << confirmation.transaction_id
                   << " and creative instance id "
                   << confirmation.creative_instance_id
                   << " to the confirmations queue");
}

void RemoveFromRetryQueue(const ConfirmationInfo& confirmation) {
  CHECK(IsValid(confirmation));

  if (!ConfirmationStateManager::GetInstance().RemoveFailedConfirmation(
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

  ConfirmationStateManager::GetInstance().Save();
}

}  // namespace

Confirmations::Confirmations(privacy::TokenGeneratorInterface* token_generator)
    : token_generator_(token_generator) {
  CHECK(token_generator_);
}

Confirmations::~Confirmations() {
  delegate_ = nullptr;
}

void Confirmations::Confirm(const TransactionInfo& transaction) {
  CHECK(transaction.IsValid());

  ConfirmTransaction(transaction);
}

void Confirmations::ProcessRetryQueue() {
  if (!retry_timer_.IsRunning()) {
    Retry();
  }
}

///////////////////////////////////////////////////////////////////////////////

void Confirmations::Retry() {
  const ConfirmationList& failed_confirmations =
      ConfirmationStateManager::GetInstance().GetFailedConfirmations();
  if (failed_confirmations.empty()) {
    BLOG(1, "No failed confirmations to retry");
    return;
  }

  CHECK(!retry_timer_.IsRunning());
  const base::Time retry_at = retry_timer_.StartWithPrivacy(
      FROM_HERE, kRetryAfter,
      base::BindOnce(&Confirmations::OnRetry, weak_factory_.GetWeakPtr()));

  BLOG(1, "Retry sending failed confirmations "
              << FriendlyDateAndTime(retry_at, /*use_sentence_style*/ true));
}

void Confirmations::OnRetry() {
  const ConfirmationList& failed_confirmations =
      ConfirmationStateManager::GetInstance().GetFailedConfirmations();
  CHECK(!failed_confirmations.empty());

  BLOG(1, "Retry sending failed confirmations");

  const ConfirmationInfo confirmation_copy = failed_confirmations.front();
  RemoveFromRetryQueue(confirmation_copy);

  if (confirmation_copy.opted_in) {
    return RecreateOptedInDynamicUserDataAndRedeem(confirmation_copy);
  }

  return Redeem(confirmation_copy);
}

void Confirmations::StopRetrying() {
  retry_timer_.Stop();
}

void Confirmations::ConfirmTransaction(const TransactionInfo& transaction) {
  BLOG(1, "Confirming " << transaction.confirmation_type << " for "
                        << transaction.ad_type << " with transaction id "
                        << transaction.id << " and creative instance id "
                        << transaction.creative_instance_id);

  BuildDynamicUserData(transaction);
}

void Confirmations::BuildDynamicUserData(const TransactionInfo& transaction) {
  const ConfirmationDynamicUserDataBuilder user_data_builder;
  user_data_builder.Build(base::BindOnce(&Confirmations::BuildFixedUserData,
                                         weak_factory_.GetWeakPtr(),
                                         transaction));
}

void Confirmations::BuildFixedUserData(
    const TransactionInfo& transaction,
    base::Value::Dict dynamic_opted_in_user_data) {
  const ConfirmationUserDataBuilder user_data_builder(transaction);
  user_data_builder.Build(base::BindOnce(
      &Confirmations::CreateAndRedeem, weak_factory_.GetWeakPtr(), transaction,
      std::move(dynamic_opted_in_user_data)));
}

void Confirmations::CreateAndRedeem(
    const TransactionInfo& transaction,
    base::Value::Dict dynamic_opted_in_user_data,
    base::Value::Dict fixed_opted_in_user_data) {
  OptedInUserDataInfo opted_in_user_data;
  opted_in_user_data.dynamic = std::move(dynamic_opted_in_user_data);
  opted_in_user_data.fixed = std::move(fixed_opted_in_user_data);

  const absl::optional<ConfirmationInfo> confirmation =
      CreateConfirmation(token_generator_, transaction, opted_in_user_data);
  if (!confirmation) {
    BLOG(0, "Failed to create confirmation");
    return;
  }

  Redeem(*confirmation);
}

void Confirmations::RecreateOptedInDynamicUserDataAndRedeem(
    const ConfirmationInfo& confirmation) {
  const ConfirmationDynamicUserDataBuilder user_data_builder;
  user_data_builder.Build(
      base::BindOnce(&Confirmations::OnRecreateOptedInDynamicUserDataAndRedeem,
                     weak_factory_.GetWeakPtr(), confirmation));
}

void Confirmations::OnRecreateOptedInDynamicUserDataAndRedeem(
    const ConfirmationInfo& confirmation,
    base::Value::Dict dynamic_opted_in_user_data) {
  if (!confirmation.opted_in) {
    return Redeem(confirmation);
  }

  ConfirmationInfo mutable_confirmation(confirmation);
  mutable_confirmation.opted_in->user_data.dynamic =
      std::move(dynamic_opted_in_user_data);
  mutable_confirmation.opted_in->credential_base64url =
      CreateOptedInCredential(mutable_confirmation);
  Redeem(mutable_confirmation);
}

void Confirmations::Redeem(const ConfirmationInfo& confirmation) {
  CHECK(IsValid(confirmation));

  RedeemConfirmationFactory::BuildAndRedeemConfirmation(
      weak_factory_.GetWeakPtr(), confirmation);
}

void Confirmations::OnDidRedeemOptedInConfirmation(
    const ConfirmationInfo& confirmation,
    const privacy::UnblindedPaymentTokenInfo& unblinded_payment_token) {
  CHECK(IsValid(confirmation));

  if (privacy::UnblindedPaymentTokenExists(unblinded_payment_token)) {
    BLOG(1, "Unblinded payment token is a duplicate");
    return OnFailedToRedeemConfirmation(confirmation,
                                        /*should_retry*/ false,
                                        /*should_backoff*/ false);
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

void Confirmations::OnDidRedeemOptedOutConfirmation(
    const ConfirmationInfo& confirmation) {
  CHECK(IsValid(confirmation));

  if (delegate_) {
    delegate_->OnDidConfirm(confirmation);
  }

  StopRetrying();

  ProcessRetryQueue();
}

void Confirmations::OnFailedToRedeemConfirmation(
    const ConfirmationInfo& confirmation,
    const bool should_retry,
    const bool should_backoff) {
  CHECK(IsValid(confirmation));

  if (should_retry) {
    AppendToRetryQueue(confirmation);
  }

  if (delegate_) {
    delegate_->OnFailedToConfirm(confirmation);
  }

  if (!should_backoff) {
    StopRetrying();
  }

  ProcessRetryQueue();
}

}  // namespace brave_ads
