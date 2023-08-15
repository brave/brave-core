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
#include "brave/components/brave_ads/core/internal/account/issuers/issuers_url_request_delegate.h"
#include "brave/components/brave_ads/core/internal/account/tokens/payment_tokens/payment_token_info.h"
#include "brave/components/brave_ads/core/internal/account/utility/redeem_payment_tokens/redeem_payment_tokens_delegate.h"
#include "brave/components/brave_ads/core/internal/account/utility/refill_confirmation_tokens/refill_confirmation_tokens_delegate.h"
#include "brave/components/brave_ads/core/internal/account/wallet/wallet_info.h"
#include "third_party/abseil-cpp/absl/types/optional.h"

namespace brave_ads {

class AdType;
class ConfirmationType;
class Confirmations;
class IssuersUrlRequest;
class RedeemPaymentTokens;
class RefillConfirmationTokens;
class TokenGeneratorInterface;
struct IssuersInfo;
struct TransactionInfo;

class Account final : public AdsClientNotifierObserver,
                      public ConfirmationDelegate,
                      public IssuersUrlRequestDelegate,
                      public RedeemPaymentTokensDelegate,
                      public RefillConfirmationTokensDelegate {
 public:
  explicit Account(TokenGeneratorInterface* token_generator);

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
               const std::string& segment,
               const AdType& ad_type,
               const ConfirmationType& confirmation_type) const;

  static void GetStatement(GetStatementOfAccountsCallback callback);

 private:
  void DepositCallback(const std::string& creative_instance_id,
                       const std::string& segment,
                       const AdType& ad_type,
                       const ConfirmationType& confirmation_type,
                       bool success,
                       double value) const;

  void ProcessDeposit(const std::string& creative_instance_id,
                      const std::string& segment,
                      double value,
                      const AdType& ad_type,
                      const ConfirmationType& confirmation_type) const;
  void ProcessDepositCallback(const std::string& creative_instance_id,
                              const AdType& ad_type,
                              const ConfirmationType& confirmation_type,
                              bool success,
                              const TransactionInfo& transaction) const;

  void SuccessfullyProcessedDeposit(const TransactionInfo& transaction) const;
  void FailedToProcessDeposit(const std::string& creative_instance_id,
                              const AdType& ad_type,
                              const ConfirmationType& confirmation_type) const;

  void Initialize();

  void InitializeConfirmations();

  void MaybeRewardUser();
  void InitializeUserRewards();
  void ShutdownUserRewards();

  void MaybeFetchIssuers() const;

  bool ShouldProcessUnclearedTransactions() const;
  void MaybeProcessUnclearedTransactions() const;

  bool ShouldRefillConfirmationTokens() const;
  void MaybeRefillConfirmationTokens() const;

  void MaybeReset();

  void NotifyDidInitializeWallet(const WalletInfo& wallet) const;
  void NotifyFailedToInitializeWallet() const;

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

  // ConfirmationDelegate:
  void OnDidConfirm(const ConfirmationInfo& confirmation) override;
  void OnFailedToConfirm(const ConfirmationInfo& confirmation) override;

  // IssuersUrlRequestDelegate:
  void OnDidFetchIssuers(const IssuersInfo& issuers) override;

  // RedeemPaymentTokensDelegate:
  void OnDidRedeemPaymentTokens(
      const PaymentTokenList& payment_tokens) override;

  // RefillConfirmationTokensDelegate:
  void OnWillRefillConfirmationTokens() override;
  void OnDidRefillConfirmationTokens() override;
  void OnFailedToRefillConfirmationTokens() override;
  void OnWillRetryRefillingConfirmationTokens(base::Time retry_at) override;
  void OnDidRetryRefillingConfirmationTokens() override;
  void OnCaptchaRequiredToRefillConfirmationTokens(
      const std::string& captcha_id) override;

  base::ObserverList<AccountObserver> observers_;

  const raw_ptr<TokenGeneratorInterface> token_generator_ =
      nullptr;  // NOT OWNED

  std::unique_ptr<Confirmations> confirmations_;

  std::unique_ptr<IssuersUrlRequest> issuers_url_request_;

  std::unique_ptr<RefillConfirmationTokens> refill_confirmation_tokens_;
  std::unique_ptr<RedeemPaymentTokens> redeem_payment_tokens_;

  absl::optional<WalletInfo> wallet_;

  base::WeakPtrFactory<Account> weak_factory_{this};
};

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ACCOUNT_ACCOUNT_H_
