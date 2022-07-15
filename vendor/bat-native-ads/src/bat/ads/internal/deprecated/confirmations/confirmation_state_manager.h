/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_DEPRECATED_CONFIRMATIONS_CONFIRMATION_STATE_MANAGER_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_DEPRECATED_CONFIRMATIONS_CONFIRMATION_STATE_MANAGER_H_

#include <memory>
#include <string>

#include "bat/ads/ads_callback.h"
#include "bat/ads/internal/account/confirmations/confirmation_info.h"
#include "bat/ads/internal/account/issuers/issuer_info.h"

namespace base {
class DictionaryValue;
class Value;
}  // namespace base

namespace ads {

namespace privacy {
class UnblindedPaymentTokens;
class UnblindedTokens;
}  // namespace privacy

class ConfirmationStateManager final {
 public:
  ConfirmationStateManager();
  ~ConfirmationStateManager();
  ConfirmationStateManager(const ConfirmationStateManager&) = delete;
  ConfirmationStateManager& operator=(const ConfirmationStateManager&) = delete;

  static ConfirmationStateManager* GetInstance();

  static bool HasInstance();

  void Initialize(InitializeCallback callback);
  bool IsInitialized() const;

  void Load();
  void Save();

  void SetIssuers(const IssuerList& issuers);
  IssuerList GetIssuers() const;

  ConfirmationList GetFailedConfirmations() const;
  void AppendFailedConfirmation(const ConfirmationInfo& confirmation);
  bool RemoveFailedConfirmation(const ConfirmationInfo& confirmation);
  void reset_failed_confirmations() { failed_confirmations_ = {}; }

  privacy::UnblindedTokens* GetUnblindedTokens() const {
    DCHECK(is_initialized_);
    return unblinded_tokens_.get();
  }

  privacy::UnblindedPaymentTokens* GetUnblindedPaymentTokens() const {
    DCHECK(is_initialized_);
    return unblinded_payment_tokens_.get();
  }

  bool is_mutated() const { return is_mutated_; }

 private:
  std::string ToJson();
  bool FromJson(const std::string& json);
  bool ParseIssuersFromDictionary(base::DictionaryValue* dictionary);

  base::Value GetFailedConfirmationsAsDictionary(
      const ConfirmationList& confirmations) const;
  bool GetFailedConfirmationsFromDictionary(base::Value* dictionary,
                                            ConfirmationList* confirmations);
  bool ParseFailedConfirmationsFromDictionary(
      base::DictionaryValue* dictionary);

  bool ParseUnblindedTokensFromDictionary(base::DictionaryValue* dictionary);

  bool ParseUnblindedPaymentTokensFromDictionary(
      base::DictionaryValue* dictionary);

  bool is_mutated_ = false;

  bool is_initialized_ = false;
  InitializeCallback callback_;

  IssuerList issuers_;

  ConfirmationList failed_confirmations_;

  std::unique_ptr<privacy::UnblindedTokens> unblinded_tokens_;
  std::unique_ptr<privacy::UnblindedPaymentTokens> unblinded_payment_tokens_;
};

}  // namespace ads

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_DEPRECATED_CONFIRMATIONS_CONFIRMATION_STATE_MANAGER_H_
