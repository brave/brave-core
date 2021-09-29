/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_ACCOUNT_CONFIRMATIONS_CONFIRMATIONS_STATE_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_ACCOUNT_CONFIRMATIONS_CONFIRMATIONS_STATE_H_

#include <memory>
#include <string>

#include "base/time/time.h"
#include "bat/ads/ads_aliases.h"
#include "bat/ads/internal/account/confirmations/confirmation_info_aliases.h"
#include "bat/ads/internal/catalog/catalog_issuers_info.h"
#include "bat/ads/transaction_info_aliases.h"

namespace base {
class DictionaryValue;
class Value;
}  // namespace base

namespace ads {

class AdRewards;

namespace privacy {
class UnblindedTokens;
}  // namespace privacy

class ConfirmationsState final {
 public:
  explicit ConfirmationsState(AdRewards* ad_rewards);
  ~ConfirmationsState();

  static ConfirmationsState* Get();

  static bool HasInstance();

  void Initialize(InitializeCallback callback);

  void Load();
  void Save();

  CatalogIssuersInfo GetCatalogIssuers() const;
  void SetCatalogIssuers(const CatalogIssuersInfo& catalog_issuers);

  ConfirmationList GetFailedConfirmations() const;
  void AppendFailedConfirmation(const ConfirmationInfo& confirmation);
  bool RemoveFailedConfirmation(const ConfirmationInfo& confirmation);
  void reset_failed_confirmations() { failed_confirmations_ = {}; }

  TransactionList GetTransactions() const;
  void AppendTransaction(const TransactionInfo& transaction);
  void reset_transactions() { transactions_ = {}; }

  base::Time GetNextTokenRedemptionDate() const;
  void SetNextTokenRedemptionDate(const base::Time& next_token_redemption_date);

  privacy::UnblindedTokens* get_unblinded_tokens() const {
    DCHECK(is_initialized_);
    return unblinded_tokens_.get();
  }

  privacy::UnblindedTokens* get_unblinded_payment_tokens() const {
    DCHECK(is_initialized_);
    return unblinded_payment_tokens_.get();
  }

 private:
  bool is_initialized_ = false;
  InitializeCallback callback_;

  AdRewards* ad_rewards_ = nullptr;  // NOT OWNED

  std::string ToJson();
  bool FromJson(const std::string& json);

  CatalogIssuersInfo catalog_issuers_;
  bool ParseCatalogIssuersFromDictionary(base::DictionaryValue* dictionary);

  ConfirmationList failed_confirmations_;
  base::Value GetFailedConfirmationsAsDictionary(
      const ConfirmationList& confirmations) const;
  bool GetFailedConfirmationsFromDictionary(base::Value* dictionary,
                                            ConfirmationList* confirmations);
  bool ParseFailedConfirmationsFromDictionary(
      base::DictionaryValue* dictionary);

  TransactionList transactions_;
  base::Value GetTransactionsAsDictionary(
      const TransactionList& transactions) const;
  bool GetTransactionsFromDictionary(base::Value* dictionary,
                                     TransactionList* transactions);
  bool ParseTransactionsFromDictionary(base::DictionaryValue* dictionary);

  base::Time next_token_redemption_date_;
  bool ParseNextTokenRedemptionDateFromDictionary(
      base::DictionaryValue* dictionary);

  bool ParseAdRewardsFromDictionary(base::DictionaryValue* dictionary);

  std::unique_ptr<privacy::UnblindedTokens> unblinded_tokens_;
  bool ParseUnblindedTokensFromDictionary(base::DictionaryValue* dictionary);

  std::unique_ptr<privacy::UnblindedTokens> unblinded_payment_tokens_;
  bool ParseUnblindedPaymentTokensFromDictionary(
      base::DictionaryValue* dictionary);
};

}  // namespace ads

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_ACCOUNT_CONFIRMATIONS_CONFIRMATIONS_STATE_H_
