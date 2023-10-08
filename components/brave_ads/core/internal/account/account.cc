/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/account/account.h"

#include <utility>

#include "base/check_op.h"
#include "base/functional/bind.h"
#include "brave/components/brave_ads/core/internal/account/confirmations/confirmation_info.h"
#include "brave/components/brave_ads/core/internal/account/confirmations/confirmations.h"
#include "brave/components/brave_ads/core/internal/account/confirmations/confirmations_util.h"
#include "brave/components/brave_ads/core/internal/account/deposits/deposits_factory.h"
#include "brave/components/brave_ads/core/internal/account/issuers/issuers_info.h"
#include "brave/components/brave_ads/core/internal/account/issuers/issuers_url_request.h"
#include "brave/components/brave_ads/core/internal/account/issuers/issuers_util.h"
#include "brave/components/brave_ads/core/internal/account/statement/statement.h"
#include "brave/components/brave_ads/core/internal/account/tokens/token_generator_interface.h"
#include "brave/components/brave_ads/core/internal/account/transactions/transaction_info.h"
#include "brave/components/brave_ads/core/internal/account/transactions/transactions.h"
#include "brave/components/brave_ads/core/internal/account/transactions/transactions_database_table.h"
#include "brave/components/brave_ads/core/internal/account/utility/redeem_payment_tokens/redeem_payment_tokens.h"
#include "brave/components/brave_ads/core/internal/account/utility/refill_confirmation_tokens/refill_confirmation_tokens.h"
#include "brave/components/brave_ads/core/internal/account/wallet/wallet_util.h"
#include "brave/components/brave_ads/core/internal/client/ads_client_util.h"
#include "brave/components/brave_ads/core/internal/common/logging_util.h"
#include "brave/components/brave_ads/core/internal/common/time/time_formatting_util.h"
#include "brave/components/brave_ads/core/internal/settings/settings.h"
#include "brave/components/brave_ads/core/mojom/brave_ads.mojom.h"  // IWYU pragma: keep
#include "brave/components/brave_ads/core/public/prefs/pref_names.h"
#include "brave/components/brave_ads/core/public/units/ad_type.h"
#include "brave/components/brave_rewards/common/pref_names.h"

namespace brave_ads {

namespace {

bool ShouldReset() {
  return UserHasJoinedBraveRewards() &&
         GetProfileBooleanPref(prefs::kShouldMigrateVerifiedRewardsUser);
}

void UpdateIssuers(const IssuersInfo& issuers) {
  if (!HasIssuersChanged(issuers)) {
    return BLOG(1, "Issuers already up to date");
  }

  BLOG(1, "Updated issuers");
  SetIssuers(issuers);
}

}  // namespace

Account::Account(TokenGeneratorInterface* token_generator)
    : token_generator_(token_generator) {
  CHECK(token_generator_);

  AddAdsClientNotifierObserver(this);

  InitializeConfirmations();
}

Account::~Account() {
  RemoveAdsClientNotifierObserver(this);
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
  const absl::optional<WalletInfo> wallet = ToWallet(payment_id, recovery_seed);
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
                      const std::string& segment,
                      const AdType& ad_type,
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
                     creative_instance_id, segment, ad_type,
                     confirmation_type));
}

// static
void Account::GetStatement(GetStatementOfAccountsCallback callback) {
  if (!UserHasJoinedBraveRewards()) {
    return std::move(callback).Run(/*statement=*/nullptr);
  }

  return BuildStatement(std::move(callback));
}

///////////////////////////////////////////////////////////////////////////////

void Account::DepositCallback(const std::string& creative_instance_id,
                              const std::string& segment,
                              const AdType& ad_type,
                              const ConfirmationType& confirmation_type,
                              const bool success,
                              const double value) const {
  if (!success) {
    return FailedToProcessDeposit(creative_instance_id, ad_type,
                                  confirmation_type);
  }

  ProcessDeposit(creative_instance_id, segment, value, ad_type,
                 confirmation_type);
}

void Account::ProcessDeposit(const std::string& creative_instance_id,
                             const std::string& segment,
                             const double value,
                             const AdType& ad_type,
                             const ConfirmationType& confirmation_type) const {
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

void Account::Initialize() {
  MaybeReset();

  MaybeRewardUser();

  NotifyStatementOfAccountsDidChange();

  MaybeFetchIssuers();

  MaybeProcessUnclearedTransactions();
}

void Account::InitializeConfirmations() {
  BLOG(1, "Initialize confirmations");
  confirmations_ = std::make_unique<Confirmations>(token_generator_);
  confirmations_->SetDelegate(this);
}

void Account::MaybeRewardUser() {
  UserHasJoinedBraveRewards() ? InitializeUserRewards() : ShutdownUserRewards();
}

void Account::InitializeUserRewards() {
  InitializeIssuers();
  InitializeRefillConfirmationTokens();
  InitializeRedeemPaymentTokens();
}

void Account::InitializeIssuers() {
  if (issuers_url_request_) {
    return;
  }

  BLOG(1, "Initialize issuers url request");
  issuers_url_request_ = std::make_unique<IssuersUrlRequest>();
  issuers_url_request_->SetDelegate(this);
}

void Account::InitializeRefillConfirmationTokens() {
  if (refill_confirmation_tokens_) {
    return;
  }

  BLOG(1, "Initialize refill confirmation tokens");
  refill_confirmation_tokens_ =
      std::make_unique<RefillConfirmationTokens>(token_generator_);
  refill_confirmation_tokens_->SetDelegate(this);
}

void Account::InitializeRedeemPaymentTokens() {
  if (redeem_payment_tokens_) {
    return;
  }

  BLOG(1, "Initialize redeem payment tokens");
  redeem_payment_tokens_ = std::make_unique<RedeemPaymentTokens>();
  redeem_payment_tokens_->SetDelegate(this);
}

void Account::ShutdownUserRewards() {
  ShutdownIssuers();
  ShutdownRefillConfirmationTokens();
  ShutdownRedeemPaymentTokens();
}

void Account::ShutdownIssuers() {
  if (!issuers_url_request_) {
    return;
  }

  issuers_url_request_.reset();
  BLOG(1, "Shutdown issuers url request");

  ResetIssuers();
  BLOG(1, "Reset issuers");
}

void Account::ShutdownRefillConfirmationTokens() {
  if (!refill_confirmation_tokens_) {
    return;
  }

  refill_confirmation_tokens_.reset();
  BLOG(1, "Shutdown refill confirmation tokens");
}

void Account::ShutdownRedeemPaymentTokens() {
  if (!redeem_payment_tokens_) {
    return;
  }

  redeem_payment_tokens_.reset();
  BLOG(1, "Shutdown redeem payment tokens");
}

void Account::MaybeFetchIssuers() const {
  if (issuers_url_request_) {
    issuers_url_request_->PeriodicallyFetch();
  }
}

bool Account::ShouldProcessUnclearedTransactions() const {
  return wallet_ && redeem_payment_tokens_;
}

void Account::MaybeProcessUnclearedTransactions() const {
  if (ShouldProcessUnclearedTransactions()) {
    redeem_payment_tokens_->MaybeRedeemAfterDelay(*wallet_);
  }
}

bool Account::ShouldRefillConfirmationTokens() const {
  return wallet_ && refill_confirmation_tokens_;
}

void Account::MaybeReset() {
  if (!ShouldReset()) {
    return;
  }

  InitializeConfirmations();

  ResetTokens();

  ResetAndFetchIssuers();

  SetProfileBooleanPref(prefs::kShouldMigrateVerifiedRewardsUser, false);
}

void Account::ResetAndFetchIssuers() {
  ShutdownIssuers();
  InitializeIssuers();
  MaybeFetchIssuers();
}

void Account::MaybeRefillConfirmationTokens() const {
  if (ShouldRefillConfirmationTokens()) {
    refill_confirmation_tokens_->MaybeRefill(*wallet_);
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
    MaybeReset();
  }
}

void Account::OnNotifyRewardsWalletDidUpdate(const std::string& payment_id,
                                             const std::string& recovery_seed) {
  SetWallet(payment_id, recovery_seed);

  Initialize();
}

void Account::OnNotifyDidSolveAdaptiveCaptcha() {
  MaybeRefillConfirmationTokens();
}

void Account::OnDidConfirm(const ConfirmationInfo& confirmation) {
  CHECK(IsValid(confirmation));

  MaybeRefillConfirmationTokens();
}

void Account::OnFailedToConfirm(const ConfirmationInfo& confirmation) {
  CHECK(IsValid(confirmation));

  MaybeRefillConfirmationTokens();
}

void Account::OnDidFetchIssuers(const IssuersInfo& issuers) {
  if (!IsIssuersValid(issuers)) {
    return BLOG(0, "Invalid issuers");
  }

  UpdateIssuers(issuers);

  MaybeRefillConfirmationTokens();
}

void Account::OnDidRedeemPaymentTokens(const PaymentTokenList& payment_tokens) {
  const database::table::Transactions database_table;
  database_table.Update(payment_tokens, base::BindOnce([](const bool success) {
                          if (!success) {
                            return BLOG(0, "Failed to update transactions");
                          }

                          BLOG(3, "Successfully updated transactions");
                        }));
}

void Account::OnWillRefillConfirmationTokens() {
  BLOG(1, "Refill confirmation tokens");
}

void Account::OnDidRefillConfirmationTokens() {
  BLOG(1, "Successfully refilled confirmation tokens");
}

void Account::OnFailedToRefillConfirmationTokens() {
  BLOG(1, "Failed to refill confirmation tokens");
}

void Account::OnWillRetryRefillingConfirmationTokens(base::Time retry_at) {
  BLOG(1,
       "Retry refilling confirmation tokens " << FriendlyDateAndTime(retry_at));
}

void Account::OnDidRetryRefillingConfirmationTokens() {
  BLOG(1, "Retry refilling confirmation tokens");
}

void Account::OnCaptchaRequiredToRefillConfirmationTokens(
    const std::string& captcha_id) {
  if (wallet_) {
    ShowScheduledCaptchaNotification(wallet_->payment_id, captcha_id);
  }
}

}  // namespace brave_ads
