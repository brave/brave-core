/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/account/tokens/payment_tokens/payment_token_util.h"

#include "brave/components/brave_ads/core/internal/account/tokens/payment_tokens/payment_tokens.h"
#include "brave/components/brave_ads/core/internal/deprecated/confirmations/confirmation_state_manager.h"

namespace brave_ads {

namespace {

bool HasPaymentTokens() {
  return PaymentTokenCount() > 0;
}

}  // namespace

absl::optional<PaymentTokenInfo> MaybeGetPaymentToken() {
  if (!HasPaymentTokens()) {
    return absl::nullopt;
  }

  return ConfirmationStateManager::GetInstance().GetPaymentTokens().GetToken();
}

const PaymentTokenList& GetAllPaymentTokens() {
  return ConfirmationStateManager::GetInstance()
      .GetPaymentTokens()
      .GetAllTokens();
}

void AddPaymentTokens(const PaymentTokenList& payment_tokens) {
  ConfirmationStateManager::GetInstance().GetPaymentTokens().AddTokens(
      payment_tokens);

  ConfirmationStateManager::GetInstance().SaveState();
}

bool RemovePaymentToken(const PaymentTokenInfo& payment_token) {
  if (!ConfirmationStateManager::GetInstance().GetPaymentTokens().RemoveToken(
          payment_token)) {
    return false;
  }

  ConfirmationStateManager::GetInstance().SaveState();

  return true;
}

void RemovePaymentTokens(const PaymentTokenList& payment_tokens) {
  ConfirmationStateManager::GetInstance().GetPaymentTokens().RemoveTokens(
      payment_tokens);

  ConfirmationStateManager::GetInstance().SaveState();
}

void RemoveAllPaymentTokens() {
  ConfirmationStateManager::GetInstance().GetPaymentTokens().RemoveAllTokens();

  ConfirmationStateManager::GetInstance().SaveState();
}

bool PaymentTokenExists(const PaymentTokenInfo& payment_token) {
  return ConfirmationStateManager::GetInstance().GetPaymentTokens().TokenExists(
      payment_token);
}

bool PaymentTokensIsEmpty() {
  return ConfirmationStateManager::GetInstance().GetPaymentTokens().IsEmpty();
}

size_t PaymentTokenCount() {
  return ConfirmationStateManager::GetInstance().GetPaymentTokens().Count();
}

}  // namespace brave_ads
