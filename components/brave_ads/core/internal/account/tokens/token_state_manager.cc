/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/account/tokens/token_state_manager.h"

#include <utility>

#include "base/functional/bind.h"
#include "brave/components/brave_ads/core/internal/account/tokens/confirmation_tokens/confirmation_tokens_database_table.h"
#include "brave/components/brave_ads/core/internal/account/tokens/payment_tokens/payment_tokens_database_table.h"
#include "brave/components/brave_ads/core/internal/common/logging_util.h"
#include "brave/components/brave_ads/core/internal/global_state/global_state.h"

namespace brave_ads {

TokenStateManager::TokenStateManager() = default;

TokenStateManager::~TokenStateManager() = default;

// static
TokenStateManager& TokenStateManager::GetInstance() {
  return GlobalState::GetInstance()->GetTokenStateManager();
}

void TokenStateManager::LoadState(ResultCallback callback) {
  BLOG(3, "Loading token state");

  database::table::ConfirmationTokens confirmation_tokens_database_table;
  confirmation_tokens_database_table.GetAll(
      base::BindOnce(&TokenStateManager::GetAllConfirmationTokensCallback,
                     weak_factory_.GetWeakPtr(), std::move(callback)));
}

void TokenStateManager::GetAllConfirmationTokensCallback(
    ResultCallback callback,
    bool success,
    const ConfirmationTokenList& confirmation_tokens) {
  if (!success) {
    BLOG(0, "Failed to load confirmation tokens");
    return std::move(callback).Run(/*success=*/false);
  }

  confirmation_tokens_.Set(confirmation_tokens);

  database::table::PaymentTokens payment_tokens_database_table;
  payment_tokens_database_table.GetAll(
      base::BindOnce(&TokenStateManager::GetAllPaymentTokensCallback,
                     weak_factory_.GetWeakPtr(), std::move(callback)));
}

void TokenStateManager::GetAllPaymentTokensCallback(
    ResultCallback callback,
    bool success,
    const PaymentTokenList& payment_tokens) {
  if (!success) {
    BLOG(0, "Failed to load payment tokens");
    return std::move(callback).Run(/*success=*/false);
  }

  payment_tokens_.SetTokens(payment_tokens);

  BLOG(3, "Successfully loaded token state");
  is_initialized_ = true;
  std::move(callback).Run(/*success=*/true);
}

}  // namespace brave_ads
