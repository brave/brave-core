/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/account/tokens/payment_tokens/payment_token_util.h"

#include "base/functional/bind.h"
#include "brave/components/brave_ads/core/internal/account/tokens/payment_tokens/payment_token_info.h"
#include "brave/components/brave_ads/core/internal/account/tokens/payment_tokens/payment_tokens.h"
#include "brave/components/brave_ads/core/internal/account/tokens/payment_tokens/payment_tokens_database_table.h"
#include "brave/components/brave_ads/core/internal/account/tokens/token_state_manager.h"
#include "brave/components/brave_ads/core/internal/common/logging_util.h"

namespace brave_ads {

const PaymentTokenList& GetAllPaymentTokens() {
  return TokenStateManager::GetInstance().GetPaymentTokens().GetAllTokens();
}

void AddPaymentTokens(const PaymentTokenList& payment_tokens) {
  TokenStateManager::GetInstance().GetPaymentTokens().AddTokens(payment_tokens);

  database::table::PaymentTokens payment_tokens_database_table;
  payment_tokens_database_table.Save(
      payment_tokens, base::BindOnce([](bool success) {
        if (!success) {
          BLOG(0, "Failed to save payment tokens");
        }
      }));
}

void RemovePaymentTokens(const PaymentTokenList& payment_tokens) {
  TokenStateManager::GetInstance().GetPaymentTokens().RemoveTokens(
      payment_tokens);

  database::table::PaymentTokens payment_tokens_database_table;
  payment_tokens_database_table.Delete(
      payment_tokens, base::BindOnce([](bool success) {
        if (!success) {
          BLOG(0, "Failed to delete payment tokens");
        }
      }));
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
