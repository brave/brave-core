/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/account/account.h"

#include "base/check_op.h"
#include "bat/ads/internal/account/account_util.h"
#include "bat/ads/internal/account/confirmations/confirmation_info.h"
#include "bat/ads/internal/account/confirmations/confirmations.h"
#include "bat/ads/internal/account/deposits/deposits_factory.h"
#include "bat/ads/internal/account/issuers/issuer_types.h"
#include "bat/ads/internal/account/issuers/issuers.h"
#include "bat/ads/internal/account/issuers/issuers_info.h"
#include "bat/ads/internal/account/issuers/issuers_util.h"
#include "bat/ads/internal/account/statement/statement.h"
#include "bat/ads/internal/account/transactions/transactions.h"
#include "bat/ads/internal/account/transactions/transactions_database_table.h"
#include "bat/ads/internal/account/utility/redeem_unblinded_payment_tokens/redeem_unblinded_payment_tokens.h"
#include "bat/ads/internal/account/utility/refill_unblinded_tokens/refill_unblinded_tokens.h"
#include "bat/ads/internal/account/wallet/wallet.h"
#include "bat/ads/internal/account/wallet/wallet_info.h"
#include "bat/ads/internal/ads_client_helper.h"
#include "bat/ads/internal/base/logging_util.h"
#include "bat/ads/internal/deprecated/confirmations/confirmation_state_manager.h"
#include "bat/ads/internal/prefs/pref_manager.h"
#include "bat/ads/internal/privacy/tokens/token_generator_interface.h"
#include "bat/ads/internal/privacy/tokens/unblinded_tokens/unblinded_tokens.h"
#include "bat/ads/pref_names.h"
#include "bat/ads/statement_info.h"
#include "bat/ads/transaction_info.h"
#include "third_party/abseil-cpp/absl/types/optional.h"

namespace ads {

Account::Account(privacy::TokenGeneratorInterface* token_generator)
    : confirmations_(std::make_unique<Confirmations>(token_generator)),
      issuers_(std::make_unique<Issuers>()),
      redeem_unblinded_payment_tokens_(
          std::make_unique<RedeemUnblindedPaymentTokens>()),
      refill_unblinded_tokens_(
          std::make_unique<RefillUnblindedTokens>(token_generator)),
      wallet_(std::make_unique<Wallet>()) {
  PrefManager::GetInstance()->AddObserver(this);

  confirmations_->SetDelegate(this);
  issuers_->SetDelegate(this);
  redeem_unblinded_payment_tokens_->SetDelegate(this);
  refill_unblinded_tokens_->SetDelegate(this);
}

Account::~Account() {
  PrefManager::GetInstance()->RemoveObserver(this);
}

void Account::AddObserver(AccountObserver* observer) {
  DCHECK(observer);
  observers_.AddObserver(observer);
}

void Account::RemoveObserver(AccountObserver* observer) {
  DCHECK(observer);
  observers_.RemoveObserver(observer);
}

void Account::SetWallet(const std::string& id, const std::string& seed) {
  const WalletInfo last_wallet_copy = GetWallet();

  if (!wallet_->Set(id, seed)) {
    BLOG(0, "Failed to set wallet");
    NotifyInvalidWallet();
    return;
  }

  BLOG(1, "Successfully set wallet");

  const WalletInfo& wallet = GetWallet();
  NotifyWalletDidUpdate(wallet);

  if (wallet.HasChanged(last_wallet_copy)) {
    WalletDidChange(wallet);
    return;
  }

  TopUpUnblindedTokens();
}

const WalletInfo& Account::GetWallet() const {
  return wallet_->Get();
}

void Account::Process() const {
  NotifyStatementOfAccountsDidChange();

  MaybeGetIssuers();

  ProcessClearingCycle();
}

void Account::Deposit(const std::string& creative_instance_id,
                      const AdType& ad_type,
                      const ConfirmationType& confirmation_type) const {
  DCHECK(!creative_instance_id.empty());
  DCHECK_NE(AdType::kUndefined, ad_type.value());
  DCHECK_NE(ConfirmationType::kUndefined, confirmation_type.value());

  std::unique_ptr<DepositInterface> deposit =
      DepositsFactory::Build(ad_type, confirmation_type);
  if (!deposit) {
    return;
  }

  deposit->GetValue(
      creative_instance_id, [=](const bool success, const double value) {
        if (!success) {
          FailedToProcessDeposit(creative_instance_id, ad_type,
                                 confirmation_type);
          return;
        }

        ProcessDeposit(creative_instance_id, ad_type, confirmation_type, value);
      });
}

void Account::GetStatement(GetStatementCallback callback) const {
  if (!ShouldRewardUser()) {
    callback(/* success */ false, {});
    return;
  }

  return BuildStatement(
      [callback](const bool success, const StatementInfo& statement) {
        callback(success, statement);
      });
}

///////////////////////////////////////////////////////////////////////////////

void Account::MaybeGetIssuers() const {
  if (!ShouldRewardUser()) {
    return;
  }

  issuers_->MaybeFetch();
}

void Account::ProcessDeposit(const std::string& creative_instance_id,
                             const AdType& ad_type,
                             const ConfirmationType& confirmation_type,
                             const double value) const {
  transactions::Add(
      creative_instance_id, value, ad_type, confirmation_type,
      [=](const bool success, const TransactionInfo& transaction) {
        if (!success) {
          FailedToProcessDeposit(creative_instance_id, ad_type,
                                 confirmation_type);
          return;
        }

        BLOG(3, "Successfully processed deposit for "
                    << transaction.ad_type << " with creative instance id "
                    << transaction.creative_instance_id << " and "
                    << transaction.confirmation_type << " valued at "
                    << transaction.value);

        NotifyDidProcessDeposit(transaction);

        NotifyStatementOfAccountsDidChange();

        confirmations_->Confirm(transaction);
      });
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

  if (ShouldRewardUser()) {
    ProcessUnclearedTransactions();
  }
}

void Account::ProcessUnclearedTransactions() const {
  const WalletInfo& wallet = GetWallet();
  redeem_unblinded_payment_tokens_->MaybeRedeemAfterDelay(wallet);
}

void Account::WalletDidChange(const WalletInfo& wallet) const {
  BLOG(1, "Wallet changed");

  NotifyWalletDidChange(wallet);

  ResetRewards([=](const bool success) {
    if (!success) {
      BLOG(0, "Failed to reset rewards state");
      return;
    }

    BLOG(3, "Successfully reset rewards state");

    NotifyStatementOfAccountsDidChange();

    TopUpUnblindedTokens();
  });
}

void Account::TopUpUnblindedTokens() const {
  if (!ShouldRewardUser()) {
    return;
  }

  const WalletInfo& wallet = GetWallet();
  refill_unblinded_tokens_->MaybeRefill(wallet);
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

void Account::OnPrefChanged(const std::string& path) {
  if (path == prefs::kEnabled) {
    MaybeGetIssuers();
  }
}

void Account::OnDidConfirm(const ConfirmationInfo& confirmation) {
  DCHECK(confirmation.IsValid());

  TopUpUnblindedTokens();
}

void Account::OnFailedToConfirm(const ConfirmationInfo& confirmation) {
  DCHECK(confirmation.IsValid());

  TopUpUnblindedTokens();
}

void Account::OnDidFetchIssuers(const IssuersInfo& issuers) {
  const absl::optional<IssuerInfo>& issuer_optional =
      GetIssuerForType(issuers, IssuerType::kPayments);
  if (!issuer_optional) {
    BLOG(0, "Missing issuers");
    return;
  }
  const IssuerInfo& issuer = issuer_optional.value();

  if (!IsIssuerValid(issuer)) {
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
  database::table::Transactions database_table;
  database_table.Update(unblinded_payment_tokens, [](const bool success) {
    if (!success) {
      BLOG(0, "Failed to update transactions");
      return;
    }

    BLOG(3, "Successfully updated transactions");
  });
}

void Account::OnDidRefillUnblindedTokens() {
  AdsClientHelper::GetInstance()->ClearScheduledCaptcha();
}

void Account::OnCaptchaRequiredToRefillUnblindedTokens(
    const std::string& captcha_id) {
  const WalletInfo& wallet = GetWallet();
  AdsClientHelper::GetInstance()->ShowScheduledCaptchaNotification(wallet.id,
                                                                   captcha_id);
}

}  // namespace ads
