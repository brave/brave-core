/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_DEPRECATED_CONFIRMATIONS_CONFIRMATION_STATE_MANAGER_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_DEPRECATED_CONFIRMATIONS_CONFIRMATION_STATE_MANAGER_H_

#include <optional>
#include <string>

#include "base/memory/weak_ptr.h"
#include "base/values.h"
#include "brave/components/brave_ads/core/internal/account/tokens/confirmation_tokens/confirmation_tokens.h"
#include "brave/components/brave_ads/core/internal/account/tokens/payment_tokens/payment_tokens.h"
#include "brave/components/brave_ads/core/internal/account/wallet/wallet_info.h"
#include "brave/components/brave_ads/core/public/ads_callback.h"

namespace brave_ads {

class ConfirmationStateManager final {
 public:
  ConfirmationStateManager();

  ConfirmationStateManager(const ConfirmationStateManager&) = delete;
  ConfirmationStateManager& operator=(const ConfirmationStateManager&) = delete;

  ConfirmationStateManager(ConfirmationStateManager&&) noexcept = delete;
  ConfirmationStateManager& operator=(ConfirmationStateManager&&) noexcept =
      delete;

  ~ConfirmationStateManager();

  static ConfirmationStateManager& GetInstance();

  void LoadState(std::optional<WalletInfo> wallet, InitializeCallback callback);

  bool IsInitialized() const { return is_initialized_; }

  void SaveState();

  std::string ToJson();
  [[nodiscard]] bool FromJson(const std::string& json);

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
  void LoadCallback(InitializeCallback callback,
                    const std::optional<std::string>& json);

  void ParseConfirmationTokensFromDictionary(const base::Value::Dict& dict);

  void ParsePaymentTokensFromDictionary(const base::Value::Dict& dict);

  bool is_initialized_ = false;

  std::optional<WalletInfo> wallet_;

  ConfirmationTokens confirmation_tokens_;
  PaymentTokens payment_tokens_;

  base::WeakPtrFactory<ConfirmationStateManager> weak_factory_{this};
};

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_DEPRECATED_CONFIRMATIONS_CONFIRMATION_STATE_MANAGER_H_
