/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ACCOUNT_ACCOUNT_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ACCOUNT_ACCOUNT_H_

#include <memory>
#include <string>

#include "base/memory/raw_ptr.h"
#include "base/memory/weak_ptr.h"
#include "base/observer_list.h"
#include "brave/components/brave_ads/core/ads_callback.h"
#include "brave/components/brave_ads/core/ads_client_notifier_observer.h"
#include "brave/components/brave_ads/core/internal/account/account_observer.h"
#include "brave/components/brave_ads/core/internal/account/confirmations/confirmations_delegate.h"
#include "brave/components/brave_ads/core/internal/account/confirmations/redeem_unblinded_payment_tokens/redeem_unblinded_payment_tokens_delegate.h"
#include "brave/components/brave_ads/core/internal/account/confirmations/refill_unblinded_tokens/refill_unblinded_tokens_delegate.h"
#include "brave/components/brave_ads/core/internal/account/issuers/issuers_url_request_delegate.h"
#include "brave/components/brave_ads/core/internal/account/wallet/wallet_info.h"
#include "brave/components/brave_ads/core/internal/privacy/tokens/unblinded_payment_tokens/unblinded_payment_token_info.h"
#include "third_party/abseil-cpp/absl/types/optional.h"

namespace brave_ads {

namespace privacy {
class TokenGeneratorInterface;
}  // namespace privacy

class AdType;
class ConfirmationType;
class Confirmations;
class IssuersUrlRequest;
class RedeemUnblindedPaymentTokens;
class RefillUnblindedTokens;
struct IssuersInfo;
struct TransactionInfo;

class Account final : public AdsClientNotifierObserver,
                      public ConfirmationsDelegate,
                      public IssuersUrlRequestDelegate,
                      public RedeemUnblindedPaymentTokensDelegate,
                      public RefillUnblindedTokensDelegate {
 public:
  explicit Account(privacy::TokenGeneratorInterface* token_generator);

  Account(const Account&) = delete;
  Account& operator=(const Account&) = delete;

  Account(Account&&) noexcept = delete;
  Account& operator=(Account&&) noexcept = delete;

  ~Account() override;

  void AddObserver(AccountObserver* observer);
  void RemoveObserver(AccountObserver* observer);

  void SetWallet(const std::string& payment_id,
                 const std::string& recovery_seed);
  const absl::optional<WalletInfo>& GetWallet() const { return wallet_; }

  void Deposit(const std::string& creative_instance_id,
               const AdType& ad_type,
               const std::string& segment,
               const ConfirmationType& confirmation_type) const;

  static void GetStatement(GetStatementOfAccountsCallback callback);

 private:
  void Initialize();

  void InitializeConfirmations();

  void MaybeRewardUsers();
  void InitializeRewards();
  void ShutdownRewards();

  void MaybeFetchIssuers() const;

  void DepositCallback(const std::string& creative_instance_id,
                       const AdType& ad_type,
                       const std::string& segment,
                       const ConfirmationType& confirmation_type,
                       bool success,
                       double value) const;

  void ProcessDeposit(const std::string& creative_instance_id,
                      const AdType& ad_type,
                      const std::string& segment,
                      const ConfirmationType& confirmation_type,
                      double value) const;
  void ProcessDepositCallback(const std::string& creative_instance_id,
                              const AdType& ad_type,
                              const ConfirmationType& confirmation_type,
                              bool success,
                              const TransactionInfo& transaction) const;

  void SuccessfullyProcessedDeposit(const TransactionInfo& transaction) const;
  void FailedToProcessDeposit(const std::string& creative_instance_id,
                              const AdType& ad_type,
                              const ConfirmationType& confirmation_type) const;

  void ProcessClearingCycle() const;
  bool ShouldProcessUnclearedTransactions() const;
  void MaybeProcessUnclearedTransactions() const;

  void Reset() const;
  void ResetCallback(bool success) const;

  void MaybeResetConfirmationsAndIssuers();

  bool ShouldTopUpUnblindedTokens() const;
  void MaybeTopUpUnblindedTokens() const;

  void NotifyDidInitializeWallet(const WalletInfo& wallet) const;
  void NotifyFailedToInitializeWallet() const;
  void NotifyWalletDidChange(const WalletInfo& wallet) const;

  void NotifyDidProcessDeposit(const TransactionInfo& transaction) const;
  void NotifyFailedToProcessDeposit(
      const std::string& creative_instance_id,
      const AdType& ad_type,
      const ConfirmationType& confirmation_type) const;

  void NotifyStatementOfAccountsDidChange() const;

  // AdsClientNotifierObserver:
  void OnNotifyDidInitializeAds() override;
  void OnNotifyPrefDidChange(const std::string& path) override;
  void OnNotifyRewardsWalletDidUpdate(
      const std::string& payment_id,
      const std::string& recovery_seed) override;
  void OnNotifyDidSolveAdaptiveCaptcha() override;

  // ConfirmationsDelegate:
  void OnDidRedeemConfirmation(const ConfirmationInfo& confirmation) override;
  void OnFailedToRedeemConfirmation(
      const ConfirmationInfo& confirmation) override;

  // IssuersUrlRequestDelegate:
  void OnDidFetchIssuers(const IssuersInfo& issuers) override;

  // RedeemUnblindedPaymentTokensDelegate:
  void OnDidRedeemUnblindedPaymentTokens(
      const privacy::UnblindedPaymentTokenList& unblinded_payment_tokens)
      override;

  // RefillUnblindedTokensDelegate:
  void OnCaptchaRequiredToRefillUnblindedTokens(
      const std::string& captcha_id) override;

  base::ObserverList<AccountObserver> observers_;

  const raw_ptr<privacy::TokenGeneratorInterface> token_generator_ =
      nullptr;  // NOT OWNED

  std::unique_ptr<Confirmations> confirmations_;

  std::unique_ptr<IssuersUrlRequest> issuers_url_request_;

  std::unique_ptr<RefillUnblindedTokens> refill_unblinded_tokens_;
  std::unique_ptr<RedeemUnblindedPaymentTokens>
      redeem_unblinded_payment_tokens_;

  absl::optional<WalletInfo> wallet_;

  base::WeakPtrFactory<Account> weak_factory_{this};
};

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ACCOUNT_ACCOUNT_H_
