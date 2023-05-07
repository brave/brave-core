/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_DEPRECATED_CONFIRMATIONS_CONFIRMATION_STATE_MANAGER_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_DEPRECATED_CONFIRMATIONS_CONFIRMATION_STATE_MANAGER_H_

#include <string>

#include "base/memory/weak_ptr.h"
#include "base/values.h"
#include "brave/components/brave_ads/core/ads_callback.h"
#include "brave/components/brave_ads/core/internal/account/confirmations/confirmation_info.h"
#include "brave/components/brave_ads/core/internal/account/wallet/wallet_info.h"
#include "brave/components/brave_ads/core/internal/privacy/tokens/unblinded_payment_tokens/unblinded_payment_tokens.h"
#include "brave/components/brave_ads/core/internal/privacy/tokens/unblinded_tokens/unblinded_tokens.h"
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

  void Initialize(const WalletInfo& wallet, InitializeCallback callback);
  bool IsInitialized() const { return is_initialized_; }

  void Save();

  std::string ToJson();
  bool FromJson(const std::string& json);

  absl::optional<OptedInInfo> GetOptedIn(const base::Value::Dict& dict) const;
  bool GetFailedConfirmationsFromDictionary(
      const base::Value::Dict& dict,
      ConfirmationList* confirmations) const;
  const ConfirmationList& GetFailedConfirmations() const;
  void AppendFailedConfirmation(const ConfirmationInfo& confirmation);
  bool RemoveFailedConfirmation(const ConfirmationInfo& confirmation);
  void reset_failed_confirmations() { failed_confirmations_.clear(); }

  const privacy::UnblindedTokens& GetUnblindedTokens() const {
    CHECK(is_initialized_);
    return unblinded_tokens_;
  }

  privacy::UnblindedTokens& GetUnblindedTokens() {
    CHECK(is_initialized_);
    return unblinded_tokens_;
  }

  const privacy::UnblindedPaymentTokens& GetUnblindedPaymentTokens() const {
    CHECK(is_initialized_);
    return unblinded_payment_tokens_;
  }

  privacy::UnblindedPaymentTokens& GetUnblindedPaymentTokens() {
    CHECK(is_initialized_);
    return unblinded_payment_tokens_;
  }

  bool is_mutated() const { return is_mutated_; }

 private:
  void OnLoaded(InitializeCallback callback,
                bool success,
                const std::string& json);

  bool ParseFailedConfirmationsFromDictionary(const base::Value::Dict& dict);

  bool ParseUnblindedTokensFromDictionary(const base::Value::Dict& dict);

  bool ParseUnblindedPaymentTokensFromDictionary(const base::Value::Dict& dict);

  bool is_mutated_ = false;

  bool is_initialized_ = false;

  WalletInfo wallet_;

  ConfirmationList failed_confirmations_;

  privacy::UnblindedTokens unblinded_tokens_;
  privacy::UnblindedPaymentTokens unblinded_payment_tokens_;

  base::WeakPtrFactory<ConfirmationStateManager> weak_factory_{this};
};

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_DEPRECATED_CONFIRMATIONS_CONFIRMATION_STATE_MANAGER_H_
