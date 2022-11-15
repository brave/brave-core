/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/privacy/tokens/unblinded_payment_tokens/unblinded_payment_token_util.h"

#include "bat/ads/internal/deprecated/confirmations/confirmation_state_manager.h"
#include "bat/ads/internal/privacy/tokens/unblinded_payment_tokens/unblinded_payment_tokens.h"

namespace ads::privacy {

UnblindedPaymentTokenList GetUnblindedPaymentTokens(const int count) {
  return ConfirmationStateManager::GetInstance()
      ->BuildUnblindedPaymentTokens()
      ->GetTokens(count);
}

const UnblindedPaymentTokenList& GetAllUnblindedPaymentTokens() {
  return ConfirmationStateManager::GetInstance()
      ->BuildUnblindedPaymentTokens()
      ->GetAllTokens();
}

void SetUnblindedPaymentTokens(
    const UnblindedPaymentTokenList& unblinded_tokens) {
  return ConfirmationStateManager::GetInstance()
      ->BuildUnblindedPaymentTokens()
      ->SetTokens(unblinded_tokens);
}

void AddUnblindedPaymentTokens(
    const UnblindedPaymentTokenList& unblinded_tokens) {
  ConfirmationStateManager::GetInstance()
      ->BuildUnblindedPaymentTokens()
      ->AddTokens(unblinded_tokens);

  ConfirmationStateManager::GetInstance()->Save();
}

void RemoveUnblindedPaymentTokens(
    const UnblindedPaymentTokenList& unblinded_tokens) {
  ConfirmationStateManager::GetInstance()
      ->BuildUnblindedPaymentTokens()
      ->RemoveTokens(unblinded_tokens);

  ConfirmationStateManager::GetInstance()->Save();
}

void RemoveAllUnblindedPaymentTokens() {
  ConfirmationStateManager::GetInstance()
      ->BuildUnblindedPaymentTokens()
      ->RemoveAllTokens();

  ConfirmationStateManager::GetInstance()->Save();
}

bool UnblindedPaymentTokenExists(
    const UnblindedPaymentTokenInfo& unblinded_token) {
  return ConfirmationStateManager::GetInstance()
      ->BuildUnblindedPaymentTokens()
      ->TokenExists(unblinded_token);
}

bool UnblindedPaymentTokensIsEmpty() {
  return ConfirmationStateManager::GetInstance()
      ->BuildUnblindedPaymentTokens()
      ->IsEmpty();
}

int UnblindedPaymentTokenCount() {
  return ConfirmationStateManager::GetInstance()
      ->BuildUnblindedPaymentTokens()
      ->Count();
}

}  // namespace ads::privacy
