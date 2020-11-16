/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BAT_ADS_INTERNAL_CONFIRMATIONS_CONFIRMATIONS_H_
#define BAT_ADS_INTERNAL_CONFIRMATIONS_CONFIRMATIONS_H_

#include <memory>
#include <string>

#include "bat/ads/ads.h"
#include "bat/ads/internal/catalog/catalog_issuers_info.h"
#include "bat/ads/internal/confirmations/confirmations_state.h"
#include "bat/ads/internal/timer.h"
#include "bat/ads/result.h"

namespace ads {

class AdsImpl;

namespace privacy {
class UnblindedTokens;
}  // namespace privacy

class Confirmations {
 public:
  Confirmations(
      AdsImpl* ads);

  ~Confirmations();

  void Initialize(
      InitializeCallback callback);

  CatalogIssuersInfo GetCatalogIssuers() const;
  void SetCatalogIssuers(
      const CatalogIssuersInfo& catalog_issuers);

  base::Time get_next_token_redemption_date() const;
  void set_next_token_redemption_date(
      const base::Time& next_token_redemption_date);

  void ConfirmAd(
      const std::string& creative_instance_id,
      const ConfirmationType confirmation_type);

  void RetryFailedConfirmationsAfterDelay();

  TransactionList get_transactions() const;

  void AppendTransaction(
      const double estimated_redemption_value,
      const ConfirmationType confirmation_type);

  void AppendConfirmationToRetryQueue(
      const ConfirmationInfo& confirmation);

  privacy::UnblindedTokens* get_unblinded_tokens();

  privacy::UnblindedTokens* get_unblinded_payment_tokens();

  void Save();

 private:
  bool is_initialized_ = false;

  InitializeCallback callback_;

  Timer failed_confirmations_timer_;
  void RetryFailedConfirmations();
  void RemoveConfirmationFromRetryQueue(
      const ConfirmationInfo& confirmation);

  void OnSaved(
      const Result result);

  void Load();
  void OnLoaded(
      const Result result,
      const std::string& json);

  AdsImpl* ads_;  // NOT OWNED

  std::unique_ptr<ConfirmationsState> state_;
};

}  // namespace ads

#endif  // BAT_ADS_INTERNAL_CONFIRMATIONS_CONFIRMATIONS_H_
