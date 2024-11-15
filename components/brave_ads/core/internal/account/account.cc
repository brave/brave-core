/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/account/account.h"

#include <utility>

#include "base/check_op.h"
#include "base/functional/bind.h"
#include "brave/components/brave_ads/core/internal/account/account_util.h"
#include "brave/components/brave_ads/core/internal/account/confirmations/confirmation_info.h"
#include "brave/components/brave_ads/core/internal/account/confirmations/confirmations.h"
#include "brave/components/brave_ads/core/internal/account/deposits/deposit_interface.h"
#include "brave/components/brave_ads/core/internal/account/deposits/deposits_factory.h"
#include "brave/components/brave_ads/core/internal/account/statement/statement.h"
#include "brave/components/brave_ads/core/internal/account/transactions/transaction_info.h"
#include "brave/components/brave_ads/core/internal/account/transactions/transactions.h"
#include "brave/components/brave_ads/core/internal/account/user_rewards/user_rewards.h"
#include "brave/components/brave_ads/core/internal/account/wallet/wallet_util.h"
#include "brave/components/brave_ads/core/internal/ads_client/ads_client_util.h"
#include "brave/components/brave_ads/core/internal/ads_notifier_manager.h"
#include "brave/components/brave_ads/core/internal/common/logging_util.h"
#include "brave/components/brave_ads/core/internal/prefs/pref_path_util.h"
#include "brave/components/brave_ads/core/internal/settings/settings.h"
#include "brave/components/brave_ads/core/mojom/brave_ads.mojom.h"
#include "brave/components/brave_ads/core/public/ads_client/ads_client.h"

namespace brave_ads {

Account::Account() {
  GetAdsClient().AddObserver(this);

  InitializeConfirmations();
}

Account::~Account() {
  GetAdsClient().RemoveObserver(this);
}

void Account::AddObserver(AccountObserver* const observer) {
  CHECK(observer);

  observers_.AddObserver(observer);
}

void Account::RemoveObserver(AccountObserver* const observer) {
  CHECK(observer);

  observers_.RemoveObserver(observer);
}

void Account::SetWallet(const std::string& payment_id,
                        const std::string& recovery_seed_base64) {
  const std::optional<WalletInfo> wallet =
      CreateWalletFromRecoverySeed(payment_id, recovery_seed_base64);
  if (!wallet) {
    BLOG(0, "Failed to initialize wallet");

    return NotifyFailedToInitializeWallet();
  }

  wallet_ = wallet;

  BLOG(1, "Successfully initialized wallet");
  NotifyDidInitializeWallet(*wallet);
}

void Account::GetStatement(GetStatementOfAccountsCallback callback) {
  if (!UserHasJoinedBraveRewards()) {
    // No-op if the user has not joined Brave Rewards.
    return std::move(callback).Run(/*statement=*/nullptr);
  }

  return BuildStatement(std::move(callback));
}

void Account::Deposit(
    const std::string& creative_instance_id,
    const std::string& segment,
    const mojom::AdType mojom_ad_type,
    const mojom::ConfirmationType mojom_confirmation_type) const {
  DepositWithUserData(creative_instance_id, segment, mojom_ad_type,
                      mojom_confirmation_type,
                      /*user_data=*/base::Value::Dict());
}

void Account::DepositWithUserData(
    const std::string& creative_instance_id,
    const std::string& segment,
    const mojom::AdType mojom_ad_type,
    const mojom::ConfirmationType mojom_confirmation_type,
    base::Value::Dict user_data) const {
  CHECK(!creative_instance_id.empty());
  CHECK_NE(mojom::AdType::kUndefined, mojom_ad_type);
  CHECK_NE(mojom::ConfirmationType::kUndefined, mojom_confirmation_type);

  if (!IsAllowedToDeposit(mojom_ad_type, mojom_confirmation_type)) {
    return;
  }

  const std::unique_ptr<DepositInterface> deposit =
      DepositsFactory::Build(mojom_confirmation_type);
  if (!deposit) {
    return;
  }

  deposit->GetValue(
      creative_instance_id,
      base::BindOnce(&Account::DepositCallback, weak_factory_.GetWeakPtr(),
                     creative_instance_id, segment, mojom_ad_type,
                     mojom_confirmation_type, std::move(user_data)));
}

///////////////////////////////////////////////////////////////////////////////

void Account::DepositCallback(
    const std::string& creative_instance_id,
    const std::string& segment,
    const mojom::AdType mojom_ad_type,
    const mojom::ConfirmationType mojom_confirmation_type,
    base::Value::Dict user_data,
    const bool success,
    const double value) const {
  if (!success) {
    return FailedToProcessDeposit(creative_instance_id, mojom_ad_type,
                                  mojom_confirmation_type);
  }

  ProcessDeposit(creative_instance_id, segment, value, mojom_ad_type,
                 mojom_confirmation_type, std::move(user_data));
}

void Account::ProcessDeposit(
    const std::string& creative_instance_id,
    const std::string& segment,
    const double value,
    const mojom::AdType mojom_ad_type,
    const mojom::ConfirmationType mojom_confirmation_type,
    base::Value::Dict user_data) const {
  if (!UserHasJoinedBraveRewards()) {
    // If the user has not joined Brave Rewards, there's no need to record
    // transactions.
    return SuccessfullyProcessedDeposit(
        BuildTransaction(creative_instance_id, segment, value, mojom_ad_type,
                         mojom_confirmation_type),
        std::move(user_data));
  }

  AddTransaction(creative_instance_id, segment, value, mojom_ad_type,
                 mojom_confirmation_type,
                 base::BindOnce(&Account::ProcessDepositCallback,
                                weak_factory_.GetWeakPtr(),
                                creative_instance_id, mojom_ad_type,
                                mojom_confirmation_type, std::move(user_data)));
}

void Account::ProcessDepositCallback(
    const std::string& creative_instance_id,
    const mojom::AdType mojom_ad_type,
    const mojom::ConfirmationType mojom_confirmation_type,
    base::Value::Dict user_data,
    const bool success,
    const TransactionInfo& transaction) const {
  if (!success) {
    return FailedToProcessDeposit(creative_instance_id, mojom_ad_type,
                                  mojom_confirmation_type);
  }

  SuccessfullyProcessedDeposit(transaction, std::move(user_data));
}

void Account::SuccessfullyProcessedDeposit(const TransactionInfo& transaction,
                                           base::Value::Dict user_data) const {
  BLOG(3, "Successfully processed deposit for "
              << transaction.ad_type << " with creative instance id "
              << transaction.creative_instance_id << " and "
              << transaction.confirmation_type << " valued at "
              << transaction.value);

  confirmations_->Confirm(transaction, std::move(user_data));

  NotifyDidProcessDeposit(transaction);

  AdsNotifierManager::GetInstance().NotifyAdRewardsDidChange();
}

void Account::FailedToProcessDeposit(
    const std::string& creative_instance_id,
    const mojom::AdType mojom_ad_type,
    const mojom::ConfirmationType mojom_confirmation_type) const {
  BLOG(0, "Failed to process deposit for "
              << mojom_ad_type << " with creative instance id "
              << creative_instance_id << " and " << mojom_confirmation_type);

  NotifyFailedToProcessDeposit(creative_instance_id, mojom_ad_type,
                               mojom_confirmation_type);
}

void Account::Initialize() {
  MaybeInitializeUserRewards();

  AdsNotifierManager::GetInstance().NotifyAdRewardsDidChange();
}

void Account::InitializeConfirmations() {
  BLOG(1, "Initialize confirmations");

  confirmations_ = std::make_unique<Confirmations>();
  confirmations_->SetDelegate(this);
}

void Account::MaybeInitializeUserRewards() {
  if (!wallet_) {
    return;
  }

  if (user_rewards_ || !UserHasJoinedBraveRewards()) {
    return;
  }

  BLOG(1, "Initialize user rewards");

  // We do not need to destroy the `user_rewards` object when a user resets
  // Brave Rewards because the associated data and the `Ads` instance will be
  // destroyed.

  user_rewards_ = std::make_unique<UserRewards>(*wallet_);

  user_rewards_->FetchIssuers();

  user_rewards_->MaybeRedeemPaymentTokens();
}

void Account::MaybeRefillConfirmationTokens() {
  if (user_rewards_) {
    user_rewards_->MaybeRefillConfirmationTokens();
  }
}

void Account::NotifyDidInitializeWallet(const WalletInfo& wallet) const {
  for (AccountObserver& observer : observers_) {
    observer.OnDidInitializeWallet(wallet);
  }
}

void Account::NotifyFailedToInitializeWallet() const {
  for (AccountObserver& observer : observers_) {
    observer.OnFailedToInitializeWallet();
  }
}

void Account::NotifyDidProcessDeposit(
    const TransactionInfo& transaction) const {
  for (AccountObserver& observer : observers_) {
    observer.OnDidProcessDeposit(transaction);
  }
}

void Account::NotifyFailedToProcessDeposit(
    const std::string& creative_instance_id,
    const mojom::AdType mojom_ad_type,
    const mojom::ConfirmationType mojom_confirmation_type) const {
  for (AccountObserver& observer : observers_) {
    observer.OnFailedToProcessDeposit(creative_instance_id, mojom_ad_type,
                                      mojom_confirmation_type);
  }
}

void Account::OnNotifyDidInitializeAds() {
  Initialize();
}

void Account::OnNotifyPrefDidChange(const std::string& path) {
  if (DoesMatchUserHasJoinedBraveRewardsPrefPath(path)) {
    Initialize();
  }
}

void Account::OnNotifyRewardsWalletDidUpdate(
    const std::string& payment_id,
    const std::string& recovery_seed_base64) {
  SetWallet(payment_id, recovery_seed_base64);

  Initialize();
}

void Account::OnDidConfirm(const ConfirmationInfo& /*confirmation*/) {
  MaybeRefillConfirmationTokens();
}

void Account::OnFailedToConfirm(const ConfirmationInfo& /*confirmation*/) {
  MaybeRefillConfirmationTokens();
}

}  // namespace brave_ads
