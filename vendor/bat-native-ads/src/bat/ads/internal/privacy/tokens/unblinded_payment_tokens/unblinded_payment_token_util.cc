/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/privacy/tokens/unblinded_payment_tokens/unblinded_payment_token_util.h"

#include "bat/ads/internal/deprecated/confirmations/confirmation_state_manager.h"
#include "bat/ads/internal/privacy/tokens/unblinded_payment_tokens/unblinded_payment_tokens.h"

namespace ads::privacy {

namespace {

bool HasUnblindedPaymentTokens() {
  return UnblindedPaymentTokenCount() > 0;
}

}  // namespace

absl::optional<UnblindedPaymentTokenInfo> MaybeGetUnblindedPaymentToken() {
  if (!HasUnblindedPaymentTokens()) {
    return absl::nullopt;
  }

  return ConfirmationStateManager::GetInstance()
      ->GetUnblindedPaymentTokens()
      ->GetToken();
}

const UnblindedPaymentTokenList& GetAllUnblindedPaymentTokens() {
  return ConfirmationStateManager::GetInstance()
      ->GetUnblindedPaymentTokens()
      ->GetAllTokens();
}

void AddUnblindedPaymentTokens(
    const UnblindedPaymentTokenList& unblinded_tokens) {
  ConfirmationStateManager::GetInstance()
      ->GetUnblindedPaymentTokens()
      ->AddTokens(unblinded_tokens);

  ConfirmationStateManager::GetInstance()->Save();
}

bool RemoveUnblindedPaymentToken(
    const UnblindedPaymentTokenInfo& unblinded_token) {
  if (!ConfirmationStateManager::GetInstance()
           ->GetUnblindedPaymentTokens()
           ->RemoveToken(unblinded_token)) {
    return false;
  }

  ConfirmationStateManager::GetInstance()->Save();

  return true;
}

void RemoveUnblindedPaymentTokens(
    const UnblindedPaymentTokenList& unblinded_tokens) {
  ConfirmationStateManager::GetInstance()
      ->GetUnblindedPaymentTokens()
      ->RemoveTokens(unblinded_tokens);

  ConfirmationStateManager::GetInstance()->Save();
}

void RemoveAllUnblindedPaymentTokens() {
  ConfirmationStateManager::GetInstance()
      ->GetUnblindedPaymentTokens()
      ->RemoveAllTokens();

  ConfirmationStateManager::GetInstance()->Save();
}

bool UnblindedPaymentTokenExists(
    const UnblindedPaymentTokenInfo& unblinded_token) {
  return ConfirmationStateManager::GetInstance()
      ->GetUnblindedPaymentTokens()
      ->TokenExists(unblinded_token);
}

bool UnblindedPaymentTokensIsEmpty() {
  return ConfirmationStateManager::GetInstance()
      ->GetUnblindedPaymentTokens()
      ->IsEmpty();
}

int UnblindedPaymentTokenCount() {
  return ConfirmationStateManager::GetInstance()
      ->GetUnblindedPaymentTokens()
      ->Count();
}

}  // namespace ads::privacy
