/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/account/account.h"

#include <utility>
#include <vector>

#include "base/base64.h"
#include "base/check_op.h"
#include "base/functional/bind.h"
#include "brave/components/brave_ads/common/interfaces/brave_ads.mojom.h"  // IWYU pragma: keep
#include "brave/components/brave_ads/common/pref_names.h"
#include "brave/components/brave_ads/core/ad_type.h"
#include "brave/components/brave_ads/core/internal/account/account_util.h"
#include "brave/components/brave_ads/core/internal/account/confirmations/confirmation_info.h"
#include "brave/components/brave_ads/core/internal/account/confirmations/confirmation_util.h"
#include "brave/components/brave_ads/core/internal/account/confirmations/confirmations.h"
#include "brave/components/brave_ads/core/internal/account/deposits/deposits_factory.h"
#include "brave/components/brave_ads/core/internal/account/issuers/issuers.h"
#include "brave/components/brave_ads/core/internal/account/issuers/issuers_info.h"
#include "brave/components/brave_ads/core/internal/account/issuers/issuers_util.h"
#include "brave/components/brave_ads/core/internal/account/statement/statement.h"
#include "brave/components/brave_ads/core/internal/account/transactions/transaction_info.h"
#include "brave/components/brave_ads/core/internal/account/transactions/transactions.h"
#include "brave/components/brave_ads/core/internal/account/transactions/transactions_database_table.h"
#include "brave/components/brave_ads/core/internal/account/utility/redeem_unblinded_payment_tokens/redeem_unblinded_payment_tokens.h"
#include "brave/components/brave_ads/core/internal/account/utility/refill_unblinded_tokens/refill_unblinded_tokens.h"
#include "brave/components/brave_ads/core/internal/account/wallet/wallet_info.h"
#include "brave/components/brave_ads/core/internal/ads_client_helper.h"
#include "brave/components/brave_ads/core/internal/common/logging_util.h"
#include "brave/components/brave_ads/core/internal/privacy/tokens/token_generator_interface.h"
#include "third_party/abseil-cpp/absl/types/optional.h"

namespace brave_ads {

namespace {
bool ShouldResetIssuersAndConfirmations() {
  return ShouldRewardUser() && AdsClientHelper::GetInstance()->GetBooleanPref(
                                   prefs::kShouldMigrateVerifiedRewardsUser);
}

}  // namespace

Account::Account(privacy::TokenGeneratorInterface* token_generator)
    : token_generator_(token_generator) {
  CHECK(token_generator_);

  AdsClientHelper::AddObserver(this);

  Initialize();
}

Account::~Account() {
  AdsClientHelper::RemoveObserver(this);
}

void Account::AddObserver(AccountObserver* observer) {
  CHECK(observer);
  observers_.AddObserver(observer);
}

void Account::RemoveObserver(AccountObserver* observer) {
  CHECK(observer);
  observers_.RemoveObserver(observer);
}

void Account::SetWallet(const std::string& payment_id,
                        const std::string& recovery_seed) {
  const absl::optional<std::vector<uint8_t>> raw_recovery_seed =
      base::Base64Decode(recovery_seed);
  if (!raw_recovery_seed) {
    BLOG(0, "Failed to set wallet");
    return NotifyInvalidWallet();
  }

  const WalletInfo last_wallet_copy = GetWallet();

  if (!wallet_.Set(payment_id, *raw_recovery_seed)) {
    BLOG(0, "Failed to set wallet");
    return NotifyInvalidWallet();
  }

  const WalletInfo& wallet = GetWallet();

  if (wallet.WasUpdated(last_wallet_copy)) {
    WalletDidUpdate(wallet);
  }

  if (wallet.HasChanged(last_wallet_copy)) {
    return WalletDidChange(wallet);
  }

  if (wallet.WasCreated(last_wallet_copy)) {
    WalletWasCreated(wallet);

    if (!HasIssuers()) {
      return MaybeGetIssuers();
    }
  }

  TopUpUnblindedTokens();
}

const WalletInfo& Account::GetWallet() const {
  return wallet_.Get();
}

void Account::Process() {
  MaybeResetIssuersAndConfirmations();

  NotifyStatementOfAccountsDidChange();

  MaybeGetIssuers();

  ProcessClearingCycle();
}

void Account::Deposit(const std::string& creative_instance_id,
                      const AdType& ad_type,
                      const std::string& segment,
                      const ConfirmationType& confirmation_type) const {
  CHECK(!creative_instance_id.empty());
  CHECK_NE(AdType::kUndefined, ad_type);
  CHECK_NE(ConfirmationType::kUndefined, confirmation_type);

  const std::unique_ptr<DepositInterface> deposit =
      DepositsFactory::Build(ad_type, confirmation_type);
  if (!deposit) {
    return;
  }

  deposit->GetValue(
      creative_instance_id,
      base::BindOnce(&Account::OnGetDepositValue, weak_factory_.GetWeakPtr(),
                     creative_instance_id, ad_type, segment,
                     confirmation_type));
}

// static
void Account::GetStatement(GetStatementOfAccountsCallback callback) {
  if (!ShouldRewardUser()) {
    return std::move(callback).Run(/*statement*/ nullptr);
  }

  return BuildStatement(std::move(callback));
}

///////////////////////////////////////////////////////////////////////////////

void Account::Initialize() {
  confirmations_ = std::make_unique<Confirmations>(token_generator_);
  confirmations_->SetDelegate(this);

  issuers_ = std::make_unique<Issuers>();
  issuers_->SetDelegate(this);

  redeem_unblinded_payment_tokens_ =
      std::make_unique<RedeemUnblindedPaymentTokens>();
  redeem_unblinded_payment_tokens_->SetDelegate(this);

  refill_unblinded_tokens_ =
      std::make_unique<RefillUnblindedTokens>(token_generator_);
  refill_unblinded_tokens_->SetDelegate(this);
}

void Account::MaybeGetIssuers() const {
  if (!ShouldRewardUser()) {
    return;
  }

  issuers_->MaybeFetch();
}

void Account::OnGetDepositValue(const std::string& creative_instance_id,
                                const AdType& ad_type,
                                const std::string& segment,
                                const ConfirmationType& confirmation_type,
                                const bool success,
                                const double value) const {
  if (!success) {
    return FailedToProcessDeposit(creative_instance_id, ad_type,
                                  confirmation_type);
  }

  ProcessDeposit(creative_instance_id, ad_type, segment, confirmation_type,
                 value);
}

void Account::ProcessDeposit(const std::string& creative_instance_id,
                             const AdType& ad_type,
                             const std::string& segment,
                             const ConfirmationType& confirmation_type,
                             const double value) const {
  AddTransaction(
      creative_instance_id, segment, value, ad_type, confirmation_type,
      base::BindOnce(&Account::OnDepositProcessed, weak_factory_.GetWeakPtr(),
                     creative_instance_id, ad_type, confirmation_type));
}

void Account::OnDepositProcessed(const std::string& creative_instance_id,
                                 const AdType& ad_type,
                                 const ConfirmationType& confirmation_type,
                                 const bool success,
                                 const TransactionInfo& transaction) const {
  if (!success) {
    return FailedToProcessDeposit(creative_instance_id, ad_type,
                                  confirmation_type);
  }

  BLOG(3, "Successfully processed deposit for "
              << transaction.ad_type << " with creative instance id "
              << transaction.creative_instance_id << " and "
              << transaction.confirmation_type << " valued at "
              << transaction.value);

  NotifyDidProcessDeposit(transaction);

  NotifyStatementOfAccountsDidChange();

  confirmations_->Confirm(transaction);
}

void Account::FailedToProcessDeposit(
    const std::string& creative_instance_id,
    const AdType& ad_type,
    const ConfirmationType& confirmation_type) const {
  BLOG(0, "Failed to process deposit for "
              << ad_type << " with creative instance id "
              << creative_instance_id << " and " << confirmation_type);

  NotifyFailedToProcessDeposit(creative_instance_id, ad_type,
                               confirmation_type);
}

void Account::ProcessClearingCycle() const {
  confirmations_->ProcessRetryQueue();

  ProcessUnclearedTransactions();
}

void Account::ProcessUnclearedTransactions() const {
  if (!ShouldRewardUser()) {
    return;
  }

  const WalletInfo& wallet = GetWallet();
  redeem_unblinded_payment_tokens_->MaybeRedeemAfterDelay(wallet);
}

void Account::WalletWasCreated(const WalletInfo& wallet) const {
  BLOG(1, "Successfully created wallet");

  NotifyWalletWasCreated(wallet);
}

void Account::WalletDidUpdate(const WalletInfo& wallet) const {
  BLOG(1, "Successfully set wallet");

  NotifyWalletDidUpdate(wallet);
}

void Account::WalletDidChange(const WalletInfo& wallet) const {
  BLOG(1, "Wallet changed");

  NotifyWalletDidChange(wallet);

  ResetRewards(
      base::BindOnce(&Account::OnRewardsReset, weak_factory_.GetWeakPtr()));
}

void Account::OnRewardsReset(const bool success) const {
  if (!success) {
    BLOG(0, "Failed to reset rewards state");
    return;
  }

  BLOG(3, "Successfully reset rewards state");

  NotifyStatementOfAccountsDidChange();

  TopUpUnblindedTokens();
}

void Account::MaybeResetIssuersAndConfirmations() {
  if (!ShouldResetIssuersAndConfirmations()) {
    return;
  }

  Initialize();

  ResetConfirmations();

  ResetIssuers();

  AdsClientHelper::GetInstance()->SetBooleanPref(
      prefs::kShouldMigrateVerifiedRewardsUser, false);

  MaybeGetIssuers();

  ProcessClearingCycle();
}

void Account::TopUpUnblindedTokens() const {
  if (!ShouldRewardUser()) {
    return;
  }

  const WalletInfo& wallet = GetWallet();
  refill_unblinded_tokens_->MaybeRefill(wallet);
}

void Account::NotifyWalletWasCreated(const WalletInfo& wallet) const {
  for (AccountObserver& observer : observers_) {
    observer.OnWalletWasCreated(wallet);
  }
}

void Account::NotifyWalletDidUpdate(const WalletInfo& wallet) const {
  for (AccountObserver& observer : observers_) {
    observer.OnWalletDidUpdate(wallet);
  }
}

void Account::NotifyWalletDidChange(const WalletInfo& wallet) const {
  for (AccountObserver& observer : observers_) {
    observer.OnWalletDidChange(wallet);
  }
}

void Account::NotifyInvalidWallet() const {
  for (AccountObserver& observer : observers_) {
    observer.OnInvalidWallet();
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
    const AdType& ad_type,
    const ConfirmationType& confirmation_type) const {
  for (AccountObserver& observer : observers_) {
    observer.OnFailedToProcessDeposit(creative_instance_id, ad_type,
                                      confirmation_type);
  }
}

void Account::NotifyStatementOfAccountsDidChange() const {
  for (AccountObserver& observer : observers_) {
    observer.OnStatementOfAccountsDidChange();
  }
}

void Account::OnNotifyPrefDidChange(const std::string& path) {
  if (path == prefs::kEnabled) {
    MaybeResetIssuersAndConfirmations();

    MaybeGetIssuers();
  } else if (path == prefs::kShouldMigrateVerifiedRewardsUser) {
    MaybeResetIssuersAndConfirmations();
  }
}

void Account::OnDidConfirm(const ConfirmationInfo& confirmation) {
  CHECK(IsValid(confirmation));

  TopUpUnblindedTokens();
}

void Account::OnFailedToConfirm(const ConfirmationInfo& confirmation) {
  CHECK(IsValid(confirmation));

  TopUpUnblindedTokens();
}

void Account::OnDidFetchIssuers(const IssuersInfo& issuers) {
  if (!IsIssuersValid(issuers)) {
    BLOG(0, "Invalid issuers");
    return;
  }

  if (HasIssuersChanged(issuers)) {
    BLOG(1, "Updated issuers");
    SetIssuers(issuers);
  } else {
    BLOG(1, "Issuers already up to date");
  }

  TopUpUnblindedTokens();
}

void Account::OnDidRedeemUnblindedPaymentTokens(
    const privacy::UnblindedPaymentTokenList& unblinded_payment_tokens) {
  const database::table::Transactions database_table;
  database_table.Update(unblinded_payment_tokens,
                        base::BindOnce([](const bool success) {
                          if (!success) {
                            BLOG(0, "Failed to update transactions");
                            return;
                          }

                          BLOG(3, "Successfully updated transactions");
                        }));
}

void Account::OnDidRefillUnblindedTokens() {
  AdsClientHelper::GetInstance()->ClearScheduledCaptcha();
}

void Account::OnCaptchaRequiredToRefillUnblindedTokens(
    const std::string& captcha_id) {
  const WalletInfo& wallet = GetWallet();

  AdsClientHelper::GetInstance()->ShowScheduledCaptchaNotification(
      wallet.payment_id, captcha_id);
}

}  // namespace brave_ads
