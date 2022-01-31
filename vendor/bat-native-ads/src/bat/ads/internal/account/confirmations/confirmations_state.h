/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_ACCOUNT_CONFIRMATIONS_CONFIRMATIONS_STATE_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_ACCOUNT_CONFIRMATIONS_CONFIRMATIONS_STATE_H_

#include <memory>
#include <string>

#include "bat/ads/ads_aliases.h"
#include "bat/ads/internal/account/confirmations/confirmation_info_aliases.h"
#include "bat/ads/internal/account/issuers/issuer_info_aliases.h"

namespace base {
class DictionaryValue;
class Value;
}  // namespace base

namespace ads {

namespace privacy {
class UnblindedPaymentTokens;
class UnblindedTokens;
}  // namespace privacy

class ConfirmationsState final {
 public:
  ConfirmationsState();
  ~ConfirmationsState();

  static ConfirmationsState* Get();

  static bool HasInstance();

  void Initialize(InitializeCallback callback);

  void Load();
  void Save();

  void SetIssuers(const IssuerList& issuers);
  IssuerList GetIssuers() const;

  ConfirmationList GetFailedConfirmations() const;
  void AppendFailedConfirmation(const ConfirmationInfo& confirmation);
  bool RemoveFailedConfirmation(const ConfirmationInfo& confirmation);
  void reset_failed_confirmations() { failed_confirmations_ = {}; }

  privacy::UnblindedTokens* get_unblinded_tokens() const {
    DCHECK(is_initialized_);
    return unblinded_tokens_.get();
  }

  privacy::UnblindedPaymentTokens* get_unblinded_payment_tokens() const {
    DCHECK(is_initialized_);
    return unblinded_payment_tokens_.get();
  }

 private:
  bool is_initialized_ = false;
  InitializeCallback callback_;

  std::string ToJson();
  bool FromJson(const std::string& json);

  IssuerList issuers_;
  bool ParseIssuersFromDictionary(base::DictionaryValue* dictionary);

  ConfirmationList failed_confirmations_;
  base::Value GetFailedConfirmationsAsDictionary(
      const ConfirmationList& confirmations) const;
  bool GetFailedConfirmationsFromDictionary(base::Value* dictionary,
                                            ConfirmationList* confirmations);
  bool ParseFailedConfirmationsFromDictionary(
      base::DictionaryValue* dictionary);

  std::unique_ptr<privacy::UnblindedTokens> unblinded_tokens_;
  bool ParseUnblindedTokensFromDictionary(base::DictionaryValue* dictionary);

  std::unique_ptr<privacy::UnblindedPaymentTokens> unblinded_payment_tokens_;
  bool ParseUnblindedPaymentTokensFromDictionary(
      base::DictionaryValue* dictionary);
};

}  // namespace ads

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_ACCOUNT_CONFIRMATIONS_CONFIRMATIONS_STATE_H_
