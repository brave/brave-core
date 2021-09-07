/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_ACCOUNT_CONFIRMATIONS_CONFIRMATIONS_STATE_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_ACCOUNT_CONFIRMATIONS_CONFIRMATIONS_STATE_H_

#include <memory>
#include <string>

#include "base/time/time.h"
#include "base/values.h"
#include "bat/ads/ads.h"
#include "bat/ads/internal/account/confirmations/confirmation_info.h"
#include "bat/ads/internal/catalog/catalog_issuers_info.h"
#include "bat/ads/transaction_info.h"

namespace ads {

class AdRewards;

namespace privacy {
class UnblindedTokens;
}  // namespace privacy

class ConfirmationsState {
 public:
  explicit ConfirmationsState(AdRewards* ad_rewards);

  ~ConfirmationsState();

  static ConfirmationsState* Get();

  static bool HasInstance();

  void Initialize(InitializeCallback callback);

  void Load();
  void Save();

  CatalogIssuersInfo get_catalog_issuers() const;
  void set_catalog_issuers(const CatalogIssuersInfo& catalog_issuers);

  ConfirmationList get_failed_confirmations() const;
  void append_failed_confirmation(const ConfirmationInfo& confirmation);
  bool remove_failed_confirmation(const ConfirmationInfo& confirmation);
  void reset_failed_confirmations() { failed_confirmations_ = {}; }

  TransactionList get_transactions() const;
  void add_transaction(const TransactionInfo& transaction);
  void reset_transactions() { transactions_ = {}; }

  base::Time get_next_token_redemption_date() const;
  void set_next_token_redemption_date(
      const base::Time& next_token_redemption_date);

  privacy::UnblindedTokens* get_unblinded_tokens() const;

  privacy::UnblindedTokens* get_unblinded_payment_tokens() const;

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
