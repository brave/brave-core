/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ACCOUNT_ACCOUNT_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ACCOUNT_ACCOUNT_H_

#include <memory>
#include <optional>
#include <string>

#include "base/memory/raw_ptr.h"
#include "base/memory/weak_ptr.h"
#include "base/observer_list.h"
#include "base/values.h"
#include "brave/components/brave_ads/core/internal/account/account_observer.h"
#include "brave/components/brave_ads/core/internal/account/confirmations/confirmations_delegate.h"
#include "brave/components/brave_ads/core/internal/account/user_rewards/user_rewards.h"
#include "brave/components/brave_ads/core/internal/account/user_rewards/user_rewards_delegate.h"
#include "brave/components/brave_ads/core/internal/account/wallet/wallet_info.h"
#include "brave/components/brave_ads/core/public/account/confirmations/confirmation_type.h"
#include "brave/components/brave_ads/core/public/ad_units/ad_type.h"
#include "brave/components/brave_ads/core/public/ads_callback.h"
#include "brave/components/brave_ads/core/public/client/ads_client_notifier_observer.h"

namespace brave_ads {

class Confirmations;
class TokenGeneratorInterface;
struct TransactionInfo;

class Account final : public AdsClientNotifierObserver,
                      public ConfirmationDelegate,
                      public UserRewardsDelegate {
 public:
  explicit Account(TokenGeneratorInterface* token_generator);

  Account(const Account&) = delete;
  Account& operator=(const Account&) = delete;

  Account(Account&&) noexcept = delete;
  Account& operator=(Account&&) noexcept = delete;

  ~Account() override;

  void AddObserver(AccountObserver* observer);
  void RemoveObserver(AccountObserver* observer);

  bool IsUserRewardsSupported() const { return !!user_rewards_; }

  void SetWallet(const std::string& payment_id,
                 const std::string& recovery_seed_base64);

  static void GetStatement(GetStatementOfAccountsCallback callback);

  void Deposit(const std::string& creative_instance_id,
               const std::string& segment,
               AdType ad_type,
               ConfirmationType confirmation_type) const;
  void DepositWithUserData(const std::string& creative_instance_id,
                           const std::string& segment,
                           AdType ad_type,
                           ConfirmationType confirmation_type,
                           base::Value::Dict user_data) const;

 private:
  void DepositCallback(const std::string& creative_instance_id,
                       const std::string& segment,
                       AdType ad_type,
                       ConfirmationType confirmation_type,
                       base::Value::Dict user_data,
                       bool success,
                       double value) const;

  void ProcessDeposit(const std::string& creative_instance_id,
                      const std::string& segment,
                      double value,
                      AdType ad_type,
                      ConfirmationType confirmation_type,
                      base::Value::Dict user_data) const;
  void ProcessDepositCallback(const std::string& creative_instance_id,
                              AdType ad_type,
                              ConfirmationType confirmation_type,
                              base::Value::Dict user_data,
                              bool success,
                              const TransactionInfo& transaction) const;

  void SuccessfullyProcessedDeposit(const TransactionInfo& transaction,
                                    base::Value::Dict user_data) const;
  void FailedToProcessDeposit(const std::string& creative_instance_id,
                              AdType ad_type,
                              ConfirmationType confirmation_type) const;

  void Initialize();

  void InitializeConfirmations();

  void MaybeInitializeUserRewards();

  void MaybeRefillConfirmationTokens();

  void NotifyDidInitializeWallet(const WalletInfo& wallet) const;
  void NotifyFailedToInitializeWallet() const;

  void NotifyDidProcessDeposit(const TransactionInfo& transaction) const;
  void NotifyFailedToProcessDeposit(const std::string& creative_instance_id,
                                    AdType ad_type,
                                    ConfirmationType confirmation_type) const;

  // AdsClientNotifierObserver:
  void OnNotifyDidInitializeAds() override;
  void OnNotifyPrefDidChange(const std::string& path) override;
  void OnNotifyRewardsWalletDidUpdate(
      const std::string& payment_id,
      const std::string& recovery_seed) override;

  // ConfirmationDelegate:
  void OnDidConfirm(const ConfirmationInfo& confirmation) override;
  void OnFailedToConfirm(const ConfirmationInfo& confirmation) override;

  // UserRewardsDelegate:
  void OnDidMigrateVerifiedRewardsUser() override;

  const raw_ptr<TokenGeneratorInterface> token_generator_ =
      nullptr;  // NOT OWNED

  base::ObserverList<AccountObserver> observers_;

  std::unique_ptr<Confirmations> confirmations_;

  std::optional<WalletInfo> wallet_;

  std::unique_ptr<UserRewards> user_rewards_;

  base::WeakPtrFactory<Account> weak_factory_{this};
};

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ACCOUNT_ACCOUNT_H_
