/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_DEPRECATED_CONFIRMATIONS_CONFIRMATION_STATE_MANAGER_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_DEPRECATED_CONFIRMATIONS_CONFIRMATION_STATE_MANAGER_H_

#include <string>

#include "base/memory/weak_ptr.h"
#include "base/values.h"
#include "brave/components/brave_ads/core/internal/account/confirmations/confirmation_info.h"
#include "brave/components/brave_ads/core/internal/account/tokens/confirmation_tokens/confirmation_tokens.h"
#include "brave/components/brave_ads/core/internal/account/tokens/payment_tokens/payment_tokens.h"
#include "brave/components/brave_ads/core/internal/account/wallet/wallet_info.h"
#include "brave/components/brave_ads/core/public/ads_callback.h"
#include "third_party/abseil-cpp/absl/types/optional.h"

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

  void LoadState(const absl::optional<WalletInfo>& wallet,
                 InitializeCallback callback);

  bool IsInitialized() const { return is_initialized_; }

  void SaveState();

  std::string ToJson();
  [[nodiscard]] bool FromJson(const std::string& json);

  absl::optional<RewardInfo> GetReward(const base::Value::Dict& dict) const;

  bool GetConfirmationsFromDictionary(const base::Value::Dict& dict,
                                      ConfirmationList* confirmations) const;
  ConfirmationList GetConfirmations() const;
  void AddConfirmation(const ConfirmationInfo& confirmation);
  bool RemoveConfirmation(const ConfirmationInfo& confirmation);
  void reset_confirmations() { confirmations_.clear(); }

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
                    const absl::optional<std::string>& json);

  bool ParseConfirmationsFromDictionary(const base::Value::Dict& dict);

  bool ParseConfirmationTokensFromDictionary(const base::Value::Dict& dict);

  bool ParsePaymentTokensFromDictionary(const base::Value::Dict& dict);

  bool is_initialized_ = false;

  absl::optional<WalletInfo> wallet_;

  ConfirmationList confirmations_;

  ConfirmationTokens confirmation_tokens_;
  PaymentTokens payment_tokens_;

  base::WeakPtrFactory<ConfirmationStateManager> weak_factory_{this};
};

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_DEPRECATED_CONFIRMATIONS_CONFIRMATION_STATE_MANAGER_H_
