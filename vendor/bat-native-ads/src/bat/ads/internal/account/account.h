/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BAT_ADS_INTERNAL_ACCOUNT_ACCOUNT_H_
#define BAT_ADS_INTERNAL_ACCOUNT_ACCOUNT_H_

#include <stdint.h>

#include <memory>
#include <string>

#include "bat/ads/internal/account/account_observer.h"
#include "bat/ads/internal/account/ad_rewards/ad_rewards_delegate.h"
#include "bat/ads/internal/confirmations/confirmations_observer.h"
#include "bat/ads/internal/privacy/tokens/token_generator_interface.h"
#include "bat/ads/internal/tokens/redeem_unblinded_payment_tokens/redeem_unblinded_payment_tokens_delegate.h"
#include "bat/ads/transaction_info.h"

namespace ads {

class AdRewards;
class Confirmations;
class RedeemUnblindedPaymentTokens;
class RefillUnblindedTokens;
class Statement;
class Wallet;
struct StatementInfo;
struct WalletInfo;

class Account
    : public AdRewardsDelegate,
      public ConfirmationsObserver,
      public RedeemUnblindedPaymentTokensDelegate {
 public:
  Account(
      Confirmations* confirmations,
      privacy::TokenGeneratorInterface* token_generator);

  ~Account() override;

  void AddObserver(
      AccountObserver* observer);
  void RemoveObserver(
      AccountObserver* observer);

  bool SetWallet(
      const std::string& id,
      const std::string& seed);

  WalletInfo GetWallet() const;

  StatementInfo GetStatement(
      const int64_t from_timestamp,
      const int64_t to_timestamp) const;

  void Reconcile();

  void ProcessUnclearedTransactions();

  void TopUpUnblindedTokens();

 private:
  base::ObserverList<AccountObserver> observers_;

  Confirmations* confirmations_;  // NOT OWNED
  privacy::TokenGeneratorInterface* token_generator_;  // NOT_OWNED

  std::unique_ptr<Wallet> wallet_;
  std::unique_ptr<AdRewards> ad_rewards_;
  std::unique_ptr<Statement> statement_;
  std::unique_ptr<RedeemUnblindedPaymentTokens>
      redeem_unblinded_payment_tokens_;
  std::unique_ptr<RefillUnblindedTokens> refill_unblinded_tokens_;

  void NotifyWalletChanged(
      const WalletInfo& wallet);
  void NotifyWalletRestored(
      const WalletInfo& wallet);
  void NotifyWalletInvalid();
  void NotifyAdRewardsChanged();
  void NotifyTransactionsChanged();
  void NotifyUnclearedTransactionsProcessed();

  // AdRewardsDelegate implementation
  void OnDidReconcileAdRewards() override;

  // ConfirmationsObserver implementation
  void OnConfirmAd(
      const double estimated_redemption_value,
      const ConfirmationInfo& confirmation) override;
  void OnConfirmAdFailed(
      const ConfirmationInfo& confirmation) override;

  // RedeemUnblindedPaymentTokensDelegate implementation
  void OnDidRedeemUnblindedPaymentTokens() override;
  void OnFailedToRedeemUnblindedPaymentTokens() override;
  void OnDidRetryRedeemingUnblindedPaymentTokens() override;
};

}  // namespace ads

#endif  // BAT_ADS_INTERNAL_ACCOUNT_ACCOUNT_H_
