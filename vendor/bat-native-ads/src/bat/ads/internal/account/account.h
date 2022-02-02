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
#include "bat/ads/internal/account/confirmations/confirmations_delegate.h"
#include "bat/ads/internal/account/issuers/issuers_delegate.h"
#include "bat/ads/internal/account/redeem_unblinded_payment_tokens/redeem_unblinded_payment_tokens_delegate.h"
#include "bat/ads/internal/account/refill_unblinded_tokens/refill_unblinded_tokens_delegate.h"
#include "bat/ads/internal/account/statement/statement_aliases.h"
#include "bat/ads/internal/privacy/unblinded_payment_tokens/unblinded_payment_token_info_aliases.h"

namespace ads {

class AdType;
class Confirmations;
class ConfirmationType;
class Issuers;
class RedeemUnblindedPaymentTokens;
class RefillUnblindedTokens;
class Wallet;
struct CreativeAdInfo;
struct IssuersInfo;
struct WalletInfo;

namespace privacy {
class TokenGeneratorInterface;
}  // namespace privacy

class Account final : public ConfirmationsDelegate,
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

  void DepositFunds(const std::string& creative_instance_id,
                    const AdType& ad_type,
                    const ConfirmationType& confirmation_type);

  void GetStatement(StatementCallback callback) const;

  void ProcessClearingCycle();

 private:
  base::ObserverList<AccountObserver> observers_;

  std::unique_ptr<Confirmations> confirmations_;
  std::unique_ptr<Issuers> issuers_;
  std::unique_ptr<RedeemUnblindedPaymentTokens>
      redeem_unblinded_payment_tokens_;
  std::unique_ptr<RefillUnblindedTokens> refill_unblinded_tokens_;
  std::unique_ptr<Wallet> wallet_;

  void Credit(const CreativeAdInfo& creative_ad,
              const AdType& ad_type,
              const ConfirmationType& confirmation_type) const;

  void TopUpUnblindedTokens();

  void ProcessUnclearedTransactions();

  void Reset();

  void NotifyWalletDidUpdate(const WalletInfo& wallet) const;
  void NotifyWalletDidChange(const WalletInfo& wallet) const;
  void NotifyInvalidWallet() const;

  void NotifyDepositedFunds(const TransactionInfo& transaction) const;
  void NotifyFailedToDepositFunds(
      const CreativeAdInfo& creative_ad,
      const AdType& ad_type,
      const ConfirmationType& confirmation_type) const;

  void NotifyStatementOfAccountsDidChange() const;

  // ConfirmationsDelegate:
  void OnDidConfirm(const ConfirmationInfo& confirmation) override;
  void OnFailedToConfirm(const ConfirmationInfo& confirmation) override;

  // IssuersDelegate:
  void OnDidGetIssuers(const IssuersInfo& issuers) override;
  void OnFailedToGetIssuers() override;

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
