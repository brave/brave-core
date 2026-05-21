/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ACCOUNT_TOKENS_TOKEN_STATE_MANAGER_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ACCOUNT_TOKENS_TOKEN_STATE_MANAGER_H_

#include "base/check.h"
#include "base/memory/weak_ptr.h"
#include "brave/components/brave_ads/core/internal/account/tokens/confirmation_tokens/confirmation_token_info.h"
#include "brave/components/brave_ads/core/internal/account/tokens/confirmation_tokens/confirmation_tokens.h"
#include "brave/components/brave_ads/core/internal/account/tokens/payment_tokens/payment_token_info.h"
#include "brave/components/brave_ads/core/internal/account/tokens/payment_tokens/payment_tokens.h"
#include "brave/components/brave_ads/core/public/ads_callback.h"

namespace brave_ads {

// Loads confirmation tokens and payment tokens from the database into memory at
// startup and provides in-memory access to both token caches.
class TokenStateManager final {
 public:
  TokenStateManager();

  TokenStateManager(const TokenStateManager&) = delete;
  TokenStateManager& operator=(const TokenStateManager&) = delete;

  ~TokenStateManager();

  static TokenStateManager& GetInstance();

  void LoadState(ResultCallback callback);

  bool IsInitialized() const { return is_initialized_; }

  const ConfirmationTokens& GetConfirmationTokens() const {
    CHECK(is_initialized_);
    return confirmation_tokens_;
  }

  ConfirmationTokens& GetConfirmationTokens() {
    CHECK(is_initialized_);
    return confirmation_tokens_;
  }

  const PaymentTokens& GetPaymentTokens() const {
    CHECK(is_initialized_);
    return payment_tokens_;
  }

  PaymentTokens& GetPaymentTokens() {
    CHECK(is_initialized_);
    return payment_tokens_;
  }

 private:
  void GetAllConfirmationTokensCallback(
      ResultCallback callback,
      bool success,
      const ConfirmationTokenList& confirmation_tokens);

  void GetAllPaymentTokensCallback(ResultCallback callback,
                                   bool success,
                                   const PaymentTokenList& payment_tokens);

  bool is_initialized_ = false;

  ConfirmationTokens confirmation_tokens_;
  PaymentTokens payment_tokens_;

  base::WeakPtrFactory<TokenStateManager> weak_factory_{this};
};

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ACCOUNT_TOKENS_TOKEN_STATE_MANAGER_H_
