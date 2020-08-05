/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BAT_ADS_INTERNAL_CONFIRMATIONS_CCONFIRMATIONS_STATE_H_
#define BAT_ADS_INTERNAL_CONFIRMATIONS_CCONFIRMATIONS_STATE_H_

#include <stdint.h>

#include <memory>
#include <string>

#include "base/values.h"
#include "bat/ads/internal/catalog/catalog_issuers_info.h"
#include "bat/ads/internal/confirmations/confirmation_info.h"
#include "bat/ads/internal/privacy/unblinded_tokens/unblinded_tokens.h"
#include "bat/ads/internal/time_util.h"
#include "bat/ads/transaction_info.h"

namespace ads {

class AdsImpl;

class ConfirmationsState {
 public:
  ConfirmationsState(
      AdsImpl* ads);

  ~ConfirmationsState();

  std::string ToJson();
  bool FromJson(
      const std::string& json);

  CatalogIssuersInfo get_catalog_issuers() const;
  void set_catalog_issuers(
      const CatalogIssuersInfo& catalog_issuers);

  ConfirmationList get_confirmations() const;
  void append_confirmation(
      const ConfirmationInfo& confirmation);
  bool remove_confirmation(
      const ConfirmationInfo& confirmation);

  TransactionList get_transactions() const;
  void append_transaction(
      const TransactionInfo& transaction);

  base::Time get_next_token_redemption_date() const;
  void set_next_token_redemption_date(
      const base::Time& next_token_redemption_date);


  privacy::UnblindedTokens* get_unblinded_tokens() const;

  privacy::UnblindedTokens* get_unblinded_payment_tokens() const;

 private:
  AdsImpl* ads_;  // NOT OWNED

  CatalogIssuersInfo catalog_issuers_;
  bool ParseCatalogIssuersFromDictionary(
      base::DictionaryValue* dictionary);

  ConfirmationList confirmations_;
  base::Value GetConfirmationsAsDictionary(
      const ConfirmationList& confirmations) const;
  bool GetConfirmationsFromDictionary(
      base::Value* dictionary,
      ConfirmationList* confirmations);
  bool ParseConfirmationsFromDictionary(
      base::DictionaryValue* dictionary);

  TransactionList transactions_;
  base::Value GetTransactionsAsDictionary(
      const TransactionList& transactions) const;
  bool GetTransactionsFromDictionary(
      base::Value* dictionary,
      TransactionList* transactions);
  bool ParseTransactionsFromDictionary(
      base::DictionaryValue* dictionary);

  base::Time next_token_redemption_date_;
  bool ParseNextTokenRedemptionDateFromDictionary(
      base::DictionaryValue* dictionary);

  bool ParseAdRewardsFromDictionary(
      base::DictionaryValue* dictionary);

  std::unique_ptr<privacy::UnblindedTokens> unblinded_tokens_;
  bool ParseUnblindedTokensFromDictionary(
      base::DictionaryValue* dictionary);

  std::unique_ptr<privacy::UnblindedTokens> unblinded_payment_tokens_;
  bool ParseUnblindedPaymentTokensFromDictionary(
      base::DictionaryValue* dictionary);
};

}  // namespace ads

#endif  // BAT_ADS_INTERNAL_CONFIRMATIONS_CCONFIRMATIONS_STATE_H_
