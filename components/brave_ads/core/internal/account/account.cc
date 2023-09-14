/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/account/account.h"

#include <utility>
#include <vector>

#include "base/check_op.h"
#include "base/functional/bind.h"
#include "brave/components/brave_ads/common/interfaces/brave_ads.mojom.h"  // IWYU pragma: keep
#include "brave/components/brave_ads/common/pref_names.h"
#include "brave/components/brave_ads/core/ad_type.h"
#include "brave/components/brave_ads/core/internal/account/account_util.h"
#include "brave/components/brave_ads/core/internal/account/confirmations/confirmation_info.h"
#include "brave/components/brave_ads/core/internal/account/confirmations/confirmation_util.h"
#include "brave/components/brave_ads/core/internal/account/confirmations/confirmations.h"
#include "brave/components/brave_ads/core/internal/account/confirmations/redeem_unblinded_payment_tokens/redeem_unblinded_payment_tokens.h"
#include "brave/components/brave_ads/core/internal/account/confirmations/refill_unblinded_tokens/refill_unblinded_tokens.h"
#include "brave/components/brave_ads/core/internal/account/deposits/deposits_factory.h"
#include "brave/components/brave_ads/core/internal/account/issuers/issuers_info.h"
#include "brave/components/brave_ads/core/internal/account/issuers/issuers_url_request.h"
#include "brave/components/brave_ads/core/internal/account/issuers/issuers_util.h"
#include "brave/components/brave_ads/core/internal/account/statement/statement.h"
#include "brave/components/brave_ads/core/internal/account/transactions/transaction_info.h"
#include "brave/components/brave_ads/core/internal/account/transactions/transactions.h"
#include "brave/components/brave_ads/core/internal/account/transactions/transactions_database_table.h"
#include "brave/components/brave_ads/core/internal/account/wallet/wallet_util.h"
#include "brave/components/brave_ads/core/internal/ads_client_helper.h"
#include "brave/components/brave_ads/core/internal/common/logging_util.h"
#include "brave/components/brave_ads/core/internal/privacy/tokens/token_generator_interface.h"
#include "brave/components/brave_rewards/common/pref_names.h"

namespace brave_ads {

namespace {

bool ShouldResetIssuersAndConfirmations() {
  return UserHasJoinedBraveRewards() &&
         AdsClientHelper::GetInstance()->GetBooleanPref(
             prefs::kShouldMigrateVerifiedRewardsUser);
}

}  // namespace

Account::Account(privacy::TokenGeneratorInterface* token_generator)
    : token_generator_(token_generator) {
  CHECK(token_generator_);

  AdsClientHelper::AddObserver(this);

  InitializeConfirmations();
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
  const absl::optional<WalletInfo>& wallet =
      ToWallet(payment_id, recovery_seed);
  if (!wallet) {
    BLOG(0, "Failed to initialize wallet");
    return NotifyFailedToInitializeWallet();
  }

  if (wallet_ == wallet) {
    return;
  }

  wallet_ = wallet;

  BLOG(1, "Successfully initialized wallet");
  NotifyDidInitializeWallet(*wallet);
}

void Account::Deposit(const std::string& creative_instance_id,
                      const AdType& ad_type,
                      const std::string& segment,
                      const ConfirmationType& confirmation_type) const {
  CHECK(!creative_instance_id.empty());
  CHECK_NE(AdType::kUndefined, ad_type);
  CHECK_NE(ConfirmationType::kUndefined, confirmation_type);

  const std::unique_ptr<DepositInterface> deposit =
      DepositsFactory::Build(confirmation_type);
  if (!deposit) {
    return;
  }

  deposit->GetValue(
      creative_instance_id,
      base::BindOnce(&Account::DepositCallback, weak_factory_.GetWeakPtr(),
                     creative_instance_id, ad_type, segment,
                     confirmation_type));
}

// static
void Account::GetStatement(GetStatementOfAccountsCallback callback) {
  if (!UserHasJoinedBraveRewards()) {
    return std::move(callback).Run(/*statement*/ nullptr);
  }

  return BuildStatement(std::move(callback));
}

///////////////////////////////////////////////////////////////////////////////

void Account::Initialize() {
  MaybeResetConfirmationsAndIssuers();

  MaybeRewardUser();

  NotifyStatementOfAccountsDidChange();

  MaybeFetchIssuers();

  ProcessClearingCycle();
}

void Account::InitializeConfirmations() {
  confirmations_ = std::make_unique<Confirmations>(token_generator_);
  confirmations_->SetDelegate(this);
}

void Account::MaybeRewardUser() {
  UserHasJoinedBraveRewards() ? InitializeRewards() : ShutdownRewards();
}

void Account::InitializeRewards() {
  if (!issuers_url_request_) {
    BLOG(1, "Initialize issuers url request");
    issuers_url_request_ = std::make_unique<IssuersUrlRequest>();
    issuers_url_request_->SetDelegate(this);
  }

  if (!refill_unblinded_tokens_) {
    BLOG(1, "Initialize refill unblinded tokens request");
    refill_unblinded_tokens_ =
        std::make_unique<RefillUnblindedTokens>(token_generator_);
    refill_unblinded_tokens_->SetDelegate(this);
  }

  if (!redeem_unblinded_payment_tokens_) {
    BLOG(1, "Initialize redeem unblinded payment tokens request");
    redeem_unblinded_payment_tokens_ =
        std::make_unique<RedeemUnblindedPaymentTokens>();
    redeem_unblinded_payment_tokens_->SetDelegate(this);
  }
}

void Account::ShutdownRewards() {
  if (issuers_url_request_) {
    issuers_url_request_.reset();
    BLOG(1, "Shutdown issuers url request");

    ResetIssuers();
    BLOG(1, "Reset issuers");
  }

  if (refill_unblinded_tokens_) {
    refill_unblinded_tokens_.reset();
    BLOG(1, "Shutdown refill unblinded tokens request");
  }

  if (redeem_unblinded_payment_tokens_) {
    redeem_unblinded_payment_tokens_.reset();
    BLOG(1, "Shutdown redeem unblinded payment tokens request");
  }
}

void Account::MaybeFetchIssuers() const {
  if (issuers_url_request_) {
    issuers_url_request_->PeriodicallyFetch();
  }
}

void Account::DepositCallback(const std::string& creative_instance_id,
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
  if (!UserHasJoinedBraveRewards()) {
    return SuccessfullyProcessedDeposit(BuildTransaction(
        creative_instance_id, segment, value, ad_type, confirmation_type));
  }

  AddTransaction(
      creative_instance_id, segment, value, ad_type, confirmation_type,
      base::BindOnce(&Account::ProcessDepositCallback,
                     weak_factory_.GetWeakPtr(), creative_instance_id, ad_type,
                     confirmation_type));
}

void Account::ProcessDepositCallback(const std::string& creative_instance_id,
                                     const AdType& ad_type,
                                     const ConfirmationType& confirmation_type,
                                     const bool success,
                                     const TransactionInfo& transaction) const {
  if (!success) {
    return FailedToProcessDeposit(creative_instance_id, ad_type,
                                  confirmation_type);
  }

  SuccessfullyProcessedDeposit(transaction);
}

void Account::SuccessfullyProcessedDeposit(
    const TransactionInfo& transaction) const {
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

  MaybeProcessUnclearedTransactions();
}

bool Account::ShouldProcessUnclearedTransactions() const {
  return wallet_ && redeem_unblinded_payment_tokens_;
}

void Account::MaybeProcessUnclearedTransactions() const {
  if (ShouldProcessUnclearedTransactions()) {
    redeem_unblinded_payment_tokens_->MaybeRedeemAfterDelay(*wallet_);
  }
}

void Account::MaybeResetConfirmationsAndIssuers() {
  if (!ShouldResetIssuersAndConfirmations()) {
    return;
  }

  ResetConfirmations();
  InitializeConfirmations();

  ResetIssuers();

  AdsClientHelper::GetInstance()->SetBooleanPref(
      prefs::kShouldMigrateVerifiedRewardsUser, false);

  MaybeFetchIssuers();
}

bool Account::ShouldTopUpUnblindedTokens() const {
  return wallet_ && refill_unblinded_tokens_;
}

void Account::MaybeTopUpUnblindedTokens() const {
  if (ShouldTopUpUnblindedTokens()) {
    refill_unblinded_tokens_->MaybeRefill(*wallet_);
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

void Account::OnNotifyDidInitializeAds() {
  Initialize();
}

void Account::OnNotifyPrefDidChange(const std::string& path) {
  if (path == brave_rewards::prefs::kEnabled) {
    Initialize();
  } else if (path == prefs::kShouldMigrateVerifiedRewardsUser) {
    MaybeResetConfirmationsAndIssuers();
  }
}

void Account::OnNotifyRewardsWalletDidUpdate(const std::string& payment_id,
                                             const std::string& recovery_seed) {
  SetWallet(payment_id, recovery_seed);

  Initialize();
}

void Account::OnNotifyDidSolveAdaptiveCaptcha() {
  MaybeTopUpUnblindedTokens();
}

void Account::OnDidRedeemConfirmation(const ConfirmationInfo& confirmation) {
  CHECK(IsValid(confirmation));

  MaybeTopUpUnblindedTokens();
}

void Account::OnFailedToRedeemConfirmation(
    const ConfirmationInfo& confirmation) {
  CHECK(IsValid(confirmation));

  MaybeTopUpUnblindedTokens();
}

void Account::OnDidFetchIssuers(const IssuersInfo& issuers) {
  if (!IsIssuersValid(issuers)) {
    return BLOG(0, "Invalid issuers");
  }

  if (HasIssuersChanged(issuers)) {
    BLOG(1, "Updated issuers");
    SetIssuers(issuers);
  } else {
    BLOG(1, "Issuers already up to date");
  }

  MaybeTopUpUnblindedTokens();
}

void Account::OnDidRedeemUnblindedPaymentTokens(
    const privacy::UnblindedPaymentTokenList& unblinded_payment_tokens) {
  const database::table::Transactions database_table;
  database_table.Update(unblinded_payment_tokens,
                        base::BindOnce([](const bool success) {
                          if (!success) {
                            return BLOG(0, "Failed to update transactions");
                          }

                          BLOG(3, "Successfully updated transactions");
                        }));
}

void Account::OnCaptchaRequiredToRefillUnblindedTokens(
    const std::string& captcha_id) {
  if (wallet_) {
    AdsClientHelper::GetInstance()->ShowScheduledCaptchaNotification(
        wallet_->payment_id, captcha_id);
  }
}

}  // namespace brave_ads
