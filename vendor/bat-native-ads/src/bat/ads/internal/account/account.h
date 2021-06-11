/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_ACCOUNT_ACCOUNT_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_ACCOUNT_ACCOUNT_H_

#include <cstdint>
#include <memory>
#include <string>

#include "bat/ads/internal/account/account_observer.h"
#include "bat/ads/internal/account/ad_rewards/ad_rewards_delegate.h"
#include "bat/ads/internal/account/confirmations/confirmations_observer.h"
#include "bat/ads/internal/privacy/tokens/token_generator_interface.h"
#include "bat/ads/internal/tokens/redeem_unblinded_payment_tokens/redeem_unblinded_payment_tokens_delegate.h"
#include "bat/ads/transaction_info.h"

namespace ads {

class AdRewards;
class Confirmations;
class ConfirmationType;
class RedeemUnblindedPaymentTokens;
class RefillUnblindedTokens;
class Statement;
class Wallet;
struct CatalogIssuersInfo;
struct StatementInfo;
struct WalletInfo;

class Account : public AdRewardsDelegate,
                public ConfirmationsObserver,
                public RedeemUnblindedPaymentTokensDelegate {
 public:
  explicit Account(privacy::TokenGeneratorInterface* token_generator);

  ~Account() override;

  void AddObserver(AccountObserver* observer);
  void RemoveObserver(AccountObserver* observer);

  bool SetWallet(const std::string& id, const std::string& seed);

  WalletInfo GetWallet() const;

  void SetCatalogIssuers(const CatalogIssuersInfo& catalog_issuers);

  void Deposit(const std::string& creative_instance_id,
               const ConfirmationType& confirmation_type);

  StatementInfo GetStatement(const int64_t from_timestamp,
                             const int64_t to_timestamp) const;

  void Reconcile();

  void ProcessTransactions();

  void TopUpUnblindedTokens();

 private:
  base::ObserverList<AccountObserver> observers_;

  privacy::TokenGeneratorInterface* token_generator_;  // NOT_OWNED

  std::unique_ptr<AdRewards> ad_rewards_;
  std::unique_ptr<Confirmations> confirmations_;
  std::unique_ptr<RedeemUnblindedPaymentTokens>
      redeem_unblinded_payment_tokens_;
  std::unique_ptr<RefillUnblindedTokens> refill_unblinded_tokens_;
  std::unique_ptr<Statement> statement_;
  std::unique_ptr<Wallet> wallet_;

  void ProcessUnclearedTransactions();

  void NotifyWalletChanged(const WalletInfo& wallet) const;
  void NotifyWalletRestored(const WalletInfo& wallet) const;
  void NotifyWalletInvalid() const;
  void NotifyCatalogIssuersChanged(
      const CatalogIssuersInfo& catalog_issuers) const;
  void NotifyAdRewardsChanged() const;
  void NotifyTransactionsChanged() const;
  void NotifyUnclearedTransactionsProcessed() const;

  // AdRewardsDelegate implementation
  void OnDidReconcileAdRewards() override;

  // ConfirmationsObserver implementation
  void OnConfirmAd(const double estimated_redemption_value,
                   const ConfirmationInfo& confirmation) override;
  void OnConfirmAdFailed(const ConfirmationInfo& confirmation) override;

  // RedeemUnblindedPaymentTokensDelegate implementation
  void OnDidRedeemUnblindedPaymentTokens() override;
  void OnFailedToRedeemUnblindedPaymentTokens() override;
  void OnDidRetryRedeemingUnblindedPaymentTokens() override;
};

}  // namespace ads

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_ACCOUNT_ACCOUNT_H_
