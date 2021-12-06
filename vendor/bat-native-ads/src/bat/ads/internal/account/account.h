/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_ACCOUNT_ACCOUNT_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_ACCOUNT_ACCOUNT_H_

#include <memory>
#include <string>

#include "base/observer_list.h"
#include "bat/ads/internal/account/account_observer.h"
#include "bat/ads/internal/account/ad_rewards/ad_rewards_delegate.h"
#include "bat/ads/internal/account/confirmations/confirmations_observer.h"
#include "bat/ads/internal/privacy/unblinded_tokens/unblinded_token_info_aliases.h"
#include "bat/ads/internal/tokens/issuers/issuers_delegate.h"
#include "bat/ads/internal/tokens/redeem_unblinded_payment_tokens/redeem_unblinded_payment_tokens_delegate.h"
#include "bat/ads/internal/tokens/refill_unblinded_tokens/refill_unblinded_tokens_delegate.h"

namespace base {
class Time;
}  // namespace base

namespace ads {

class AdRewards;
class AdType;
class Confirmations;
class ConfirmationType;
class Issuers;
class RedeemUnblindedPaymentTokens;
class RefillUnblindedTokens;
class Statement;
class Wallet;
struct IssuersInfo;
struct StatementInfo;
struct WalletInfo;

namespace privacy {
class TokenGeneratorInterface;
}  // namespace privacy

class Account final : public AdRewardsDelegate,
                      public ConfirmationsObserver,
                      public IssuersDelegate,
                      public RedeemUnblindedPaymentTokensDelegate,
                      public RefillUnblindedTokensDelegate {
 public:
  explicit Account(privacy::TokenGeneratorInterface* token_generator);
  ~Account() override;

  void AddObserver(AccountObserver* observer);
  void RemoveObserver(AccountObserver* observer);

  bool SetWallet(const std::string& id, const std::string& seed);

  WalletInfo GetWallet() const;

  void MaybeGetIssuers() const;

  void Deposit(const std::string& creative_instance_id,
               const AdType& ad_type,
               const ConfirmationType& confirmation_type);

  StatementInfo GetStatement(const base::Time& from,
                             const base::Time& to) const;

  void Reconcile();

  void ProcessTransactions();

 private:
  base::ObserverList<AccountObserver> observers_;

  std::unique_ptr<AdRewards> ad_rewards_;
  std::unique_ptr<Issuers> issuers_;
  std::unique_ptr<Confirmations> confirmations_;
  std::unique_ptr<RedeemUnblindedPaymentTokens>
      redeem_unblinded_payment_tokens_;
  std::unique_ptr<RefillUnblindedTokens> refill_unblinded_tokens_;
  std::unique_ptr<Statement> statement_;
  std::unique_ptr<Wallet> wallet_;

  void TopUpUnblindedTokens();

  void ProcessUnclearedTransactions();

  void NotifyWalletDidUpdate(const WalletInfo& wallet) const;
  void NotifyWalletDidChange(const WalletInfo& wallet) const;
  void NotifyInvalidWallet() const;
  void NotifyStatementOfAccountsDidChange() const;

  // AdRewardsDelegate:
  void OnDidReconcileAdRewards() override;

  // IssuersDelegate:
  void OnDidGetIssuers(const IssuersInfo& issuers) override;
  void OnFailedToGetIssuers() override;

  // ConfirmationsObserver:
  void OnDidConfirm(const double estimated_redemption_value,
                    const ConfirmationInfo& confirmation) override;
  void OnFailedToConfirm(const ConfirmationInfo& confirmation) override;

  // RedeemUnblindedPaymentTokensDelegate:
  void OnDidRedeemUnblindedPaymentTokens(
      const privacy::UnblindedPaymentTokenList& unblinded_payment_tokens)
      override;
  void OnFailedToRedeemUnblindedPaymentTokens() override;
  void OnDidRetryRedeemingUnblindedPaymentTokens() override;

  // RedeemUnblindedTokensDelegate:
  void OnDidRefillUnblindedTokens() override;
  void OnCaptchaRequiredToRefillUnblindedTokens(
      const std::string& captcha_id) override;
};

}  // namespace ads

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_ACCOUNT_ACCOUNT_H_
