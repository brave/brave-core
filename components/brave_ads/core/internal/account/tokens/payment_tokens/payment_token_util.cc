/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/account/tokens/payment_tokens/payment_token_util.h"

#include "brave/components/brave_ads/core/internal/account/tokens/payment_tokens/payment_tokens.h"
#include "brave/components/brave_ads/core/internal/account/tokens/token_state_manager.h"

namespace brave_ads {

namespace {

bool HasPaymentTokens() {
  return PaymentTokenCount() > 0;
}

}  // namespace

std::optional<PaymentTokenInfo> MaybeGetPaymentToken() {
  if (!HasPaymentTokens()) {
    return std::nullopt;
  }

  return TokenStateManager::GetInstance().GetPaymentTokens().GetToken();
}

const PaymentTokenList& GetAllPaymentTokens() {
  return TokenStateManager::GetInstance().GetPaymentTokens().GetAllTokens();
}

void AddPaymentTokens(const PaymentTokenList& payment_tokens) {
  TokenStateManager::GetInstance().GetPaymentTokens().AddTokens(payment_tokens);

  TokenStateManager::GetInstance().SaveState();
}

bool RemovePaymentToken(const PaymentTokenInfo& payment_token) {
  if (!TokenStateManager::GetInstance().GetPaymentTokens().RemoveToken(
          payment_token)) {
    return false;
  }

  TokenStateManager::GetInstance().SaveState();

  return true;
}

void RemovePaymentTokens(const PaymentTokenList& payment_tokens) {
  TokenStateManager::GetInstance().GetPaymentTokens().RemoveTokens(
      payment_tokens);

  TokenStateManager::GetInstance().SaveState();
}

void RemoveAllPaymentTokens() {
  TokenStateManager::GetInstance().GetPaymentTokens().RemoveAllTokens();

  TokenStateManager::GetInstance().SaveState();
}

bool PaymentTokenExists(const PaymentTokenInfo& payment_token) {
  return TokenStateManager::GetInstance().GetPaymentTokens().TokenExists(
      payment_token);
}

bool PaymentTokensIsEmpty() {
  return TokenStateManager::GetInstance().GetPaymentTokens().IsEmpty();
}

size_t PaymentTokenCount() {
  return TokenStateManager::GetInstance().GetPaymentTokens().Count();
}

}  // namespace brave_ads
