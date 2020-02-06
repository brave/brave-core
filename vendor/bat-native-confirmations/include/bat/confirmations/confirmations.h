/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BAT_CONFIRMATIONS_CONFIRMATIONS_H_
#define BAT_CONFIRMATIONS_CONFIRMATIONS_H_

#include <stdint.h>
#include <string>
#include <vector>
#include <memory>

#include "bat/confirmations/confirmations_client.h"
#include "bat/confirmations/export.h"
#include "bat/confirmations/ad_notification_info.h"
#include "bat/confirmations/publisher_ad_info.h"
#include "bat/confirmations/issuers_info.h"
#include "bat/confirmations/wallet_info.h"
#include "bat/ledger/ledger.h"
#include "bat/ledger/transactions_info.h"

namespace confirmations {

using TransactionInfo = ::ledger::TransactionInfo;
using TransactionsInfo = ::ledger::TransactionsInfo;

using OnGetTransactionHistoryCallback = ::ledger::GetTransactionHistoryCallback;
using OnInitializeCallback = std::function<void(bool)>;

// |_environment| indicates that URL requests should use production, staging or
// development servers but can be overridden via command-line arguments
extern ledger::Environment _environment;

// |_is_debug| indicates that the next token redemption date should be reduced
// from ~7 days to ~25 minutes. This value should be set to |false| on
// production builds and |true| on debug builds
extern bool _is_debug;

// Confirmations resource name
extern const char _confirmations_resource_name[];

class CONFIRMATIONS_EXPORT Confirmations {
 public:
  Confirmations() = default;
  virtual ~Confirmations() = default;

  static Confirmations* CreateInstance(
      ConfirmationsClient* confirmations_client);

  // Should be called from Ledger to initialize Confirmations. The callback
  // takes one argument - |true| if Confirmations was successfully initialized;
  // otherwise |false|
  virtual void Initialize(
      OnInitializeCallback callback) = 0;

  // Should be called when the wallet |payment_id| and |private_key| are set in
  // the Ledger library, e.g. initializing a wallet, creating a new wallet or
  // restoring a wallet
  virtual void SetWalletInfo(
      std::unique_ptr<WalletInfo> info) = 0;

  // Should be called when the catalog issuers are updated in Ads
  virtual void SetCatalogIssuers(
      std::unique_ptr<IssuersInfo> info) = 0;

  // Should be called to get transaction history. The callback takes one
  // argument — |TransactionsInfo| which contains a list of |TransactionInfo|
  // transactions and associated earned ads rewards
  virtual void GetTransactionHistory(
      OnGetTransactionHistoryCallback callback) = 0;

  // Should be called to confirm an ad notification was viewed, clicked,
  // dismissed or landed
  virtual void ConfirmAdNotification(
      std::unique_ptr<AdNotificationInfo> info) = 0;

  // Should be called to confirm a publisher ad was viewed, clicked or landed
  virtual void ConfirmPublisherAd(
      const PublisherAdInfo& info) = 0;

  // Should be called to confirm an action, e.g. when an ad is flagged, upvoted
  // or downvoted
  virtual void ConfirmAction(
      const std::string& uuid,
      const std::string& creative_set_id,
      const ConfirmationType& type) = 0;

  // Should be called to refresh the ads rewards UI. |should_refresh| should be
  // set to |true| to fetch the latest payment balances from the server, e.g.
  // after an ad grant is claimed
  virtual void UpdateAdsRewards(
      const bool should_refresh) = 0;

  // Should be called when the timer specified by |timer_id| should be
  // triggered. Returns |true| if the timer was successfully triggered;
  // otherwise, should return |false|
  virtual bool OnTimer(
      const uint32_t timer_id) = 0;

 private:
  // Not copyable, not assignable
  Confirmations(const Confirmations&) = delete;
  Confirmations& operator=(const Confirmations&) = delete;
};

}  // namespace confirmations

#endif  // BAT_CONFIRMATIONS_CONFIRMATIONS_H_
