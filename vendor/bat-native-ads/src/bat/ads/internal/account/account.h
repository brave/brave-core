/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_ACCOUNT_ACCOUNT_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_ACCOUNT_ACCOUNT_H_

#include <memory>
#include <string>

#include "base/memory/raw_ptr.h"
#include "base/observer_list.h"
#include "bat/ads/ads_callback.h"
#include "bat/ads/internal/account/account_observer.h"
#include "bat/ads/internal/account/confirmations/confirmations_delegate.h"
#include "bat/ads/internal/account/issuers/issuers_delegate.h"
#include "bat/ads/internal/account/utility/redeem_unblinded_payment_tokens/redeem_unblinded_payment_tokens_delegate.h"
#include "bat/ads/internal/account/utility/refill_unblinded_tokens/refill_unblinded_tokens_delegate.h"
#include "bat/ads/internal/prefs/pref_manager_observer.h"
#include "bat/ads/internal/privacy/tokens/unblinded_payment_tokens/unblinded_payment_token_info.h"

namespace ads {

namespace privacy {
class TokenGeneratorInterface;
}  // namespace privacy

class AdType;
class Confirmations;
class ConfirmationType;
class Issuers;
class RedeemUnblindedPaymentTokens;
class RefillUnblindedTokens;
class Wallet;
struct IssuersInfo;
struct TransactionInfo;
struct WalletInfo;

class Account final : public PrefManagerObserver,
                      public ConfirmationsDelegate,
                      public IssuersDelegate,
                      public RedeemUnblindedPaymentTokensDelegate,
                      public RefillUnblindedTokensDelegate {
 public:
  explicit Account(privacy::TokenGeneratorInterface* token_generator);

  Account(const Account& other) = delete;
  Account& operator=(const Account& other) = delete;

  Account(Account&& other) noexcept = delete;
  Account& operator=(Account&& other) noexcept = delete;

  ~Account() override;

  void AddObserver(AccountObserver* observer);
  void RemoveObserver(AccountObserver* observer);

  void SetWallet(const std::string& payment_id,
                 const std::string& recovery_seed);
  const WalletInfo& GetWallet() const;

  void Deposit(const std::string& creative_instance_id,
               const AdType& ad_type,
               const ConfirmationType& confirmation_type) const;

  static void GetStatement(GetStatementOfAccountsCallback callback);

  void Process();

 private:
  void MaybeGetIssuers() const;

  void OnGetDepositValue(const std::string& creative_instance_id,
                         const AdType& ad_type,
                         const ConfirmationType& confirmation_type,
                         bool success,
                         double value) const;
  void ProcessDeposit(const std::string& creative_instance_id,
                      const AdType& ad_type,
                      const ConfirmationType& confirmation_type,
                      double value) const;
  void OnDepositProcessed(const std::string& creative_instance_id,
                          const AdType& ad_type,
                          const ConfirmationType& confirmation_type,
                          bool success,
                          const TransactionInfo& transaction) const;
  void FailedToProcessDeposit(const std::string& creative_instance_id,
                              const AdType& ad_type,
                              const ConfirmationType& confirmation_type) const;

  void ProcessClearingCycle() const;
  void ProcessUnclearedTransactions() const;

  void WalletDidUpdate(const WalletInfo& wallet) const;
  void WalletDidChange(const WalletInfo& wallet) const;
  void OnRewardsReset(bool success) const;

  void MaybeResetIssuersAndConfirmations();

  void TopUpUnblindedTokens() const;

  void NotifyWalletDidUpdate(const WalletInfo& wallet) const;
  void NotifyWalletDidChange(const WalletInfo& wallet) const;
  void NotifyInvalidWallet() const;

  void NotifyDidProcessDeposit(const TransactionInfo& transaction) const;
  void NotifyFailedToProcessDeposit(
      const std::string& creative_instance_id,
      const AdType& ad_type,
      const ConfirmationType& confirmation_type) const;

  void NotifyStatementOfAccountsDidChange() const;

  // PrefManagerObserver:
  void OnPrefDidChange(const std::string& path) override;

  // ConfirmationsDelegate:
  void OnDidConfirm(const ConfirmationInfo& confirmation) override;
  void OnFailedToConfirm(const ConfirmationInfo& confirmation) override;

  // IssuersDelegate:
  void OnDidFetchIssuers(const IssuersInfo& issuers) override;

  // RedeemUnblindedPaymentTokensDelegate:
  void OnDidRedeemUnblindedPaymentTokens(
      const privacy::UnblindedPaymentTokenList& unblinded_payment_tokens)
      override;

  // RedeemUnblindedTokensDelegate:
  void OnDidRefillUnblindedTokens() override;
  void OnCaptchaRequiredToRefillUnblindedTokens(
      const std::string& captcha_id) override;

  base::ObserverList<AccountObserver> observers_;

  raw_ptr<privacy::TokenGeneratorInterface> token_generator_ =
      nullptr;  // NOT OWNED

  std::unique_ptr<Confirmations> confirmations_;
  std::unique_ptr<Issuers> issuers_;
  std::unique_ptr<RedeemUnblindedPaymentTokens>
      redeem_unblinded_payment_tokens_;
  std::unique_ptr<RefillUnblindedTokens> refill_unblinded_tokens_;
  std::unique_ptr<Wallet> wallet_;
};

}  // namespace ads

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_ACCOUNT_ACCOUNT_H_
