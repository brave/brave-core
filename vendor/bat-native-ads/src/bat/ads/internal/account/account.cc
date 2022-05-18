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
#include "bat/ads/internal/account/transactions/transactions_database_table_aliases.h"
#include "bat/ads/internal/account/utility/redeem_unblinded_payment_tokens/redeem_unblinded_payment_tokens.h"
#include "bat/ads/internal/account/utility/refill_unblinded_tokens/refill_unblinded_tokens.h"
#include "bat/ads/internal/account/wallet/wallet.h"
#include "bat/ads/internal/account/wallet/wallet_info.h"
#include "bat/ads/internal/ads_client_helper.h"
#include "bat/ads/internal/base/logging_util.h"
#include "bat/ads/internal/deprecated/confirmations/confirmations_state.h"
#include "bat/ads/internal/privacy/tokens/token_generator_interface.h"
#include "bat/ads/internal/privacy/tokens/unblinded_tokens/unblinded_tokens.h"
#include "bat/ads/pref_names.h"
#include "bat/ads/statement_info.h"
#include "bat/ads/transaction_info.h"

namespace ads {

Account::Account(privacy::TokenGeneratorInterface* token_generator)
    : confirmations_(std::make_unique<Confirmations>(token_generator)),
      issuers_(std::make_unique<Issuers>()),
      redeem_unblinded_payment_tokens_(
          std::make_unique<RedeemUnblindedPaymentTokens>()),
      refill_unblinded_tokens_(
          std::make_unique<RefillUnblindedTokens>(token_generator)),
      wallet_(std::make_unique<Wallet>()) {
  confirmations_->set_delegate(this);
  issuers_->set_delegate(this);
  redeem_unblinded_payment_tokens_->set_delegate(this);
  refill_unblinded_tokens_->set_delegate(this);
}

Account::~Account() = default;

void Account::AddObserver(AccountObserver* observer) {
  DCHECK(observer);
  observers_.AddObserver(observer);
}

void Account::RemoveObserver(AccountObserver* observer) {
  DCHECK(observer);
  observers_.RemoveObserver(observer);
}

void Account::OnPrefChanged(const std::string& path) {
  if (path == prefs::kEnabled) {
    OnEnabledPrefChanged();
  }
}

bool Account::SetWallet(const std::string& id, const std::string& seed) {
  const WalletInfo& last_wallet = wallet_->Get();

  if (!wallet_->Set(id, seed)) {
    NotifyInvalidWallet();
    return false;
  }

  const WalletInfo& wallet = wallet_->Get();

  if (last_wallet.IsValid() && last_wallet != wallet) {
    NotifyWalletDidChange(wallet);

    Reset();
  }

  NotifyWalletDidUpdate(wallet);

  return true;
}

WalletInfo Account::GetWallet() const {
  return wallet_->Get();
}

void Account::MaybeGetIssuers() const {
  if (!ShouldRewardUser()) {
    return;
  }

  issuers_->MaybeFetch();
}

void Account::Deposit(const std::string& creative_instance_id,
                      const AdType& ad_type,
                      const ConfirmationType& confirmation_type) {
  DCHECK(!creative_instance_id.empty());
  DCHECK_NE(AdType::kUndefined, ad_type.value());
  DCHECK_NE(ConfirmationType::kUndefined, confirmation_type.value());

  std::unique_ptr<DepositInterface> deposit =
      DepositsFactory::Build(confirmation_type);
  DCHECK(deposit);

  deposit->GetValue(
      creative_instance_id, [=](const bool success, const double value) {
        if (!success) {
          NotifyFailedToProcessDeposit(creative_instance_id, ad_type,
                                       confirmation_type);
          return;
        }

        ProcessDeposit(creative_instance_id, ad_type, confirmation_type, value);
      });
}

void Account::GetStatement(StatementCallback callback) const {
  return BuildStatement(
      [callback](const bool success, const StatementInfo& statement) {
        callback(success, statement);
      });
}

void Account::ProcessClearingCycle() {
  confirmations_->ProcessRetryQueue();

  if (ShouldRewardUser()) {
    ProcessUnclearedTransactions();
  }
}

///////////////////////////////////////////////////////////////////////////////

void Account::OnEnabledPrefChanged() {
  MaybeGetIssuers();
}

void Account::ProcessDeposit(const std::string& creative_instance_id,
                             const AdType& ad_type,
                             const ConfirmationType& confirmation_type,
                             const double value) const {
  transactions::Add(
      creative_instance_id, value, ad_type, confirmation_type,
      [=](const bool success, const TransactionInfo& transaction) {
        if (!success) {
          NotifyFailedToProcessDeposit(creative_instance_id, ad_type,
                                       confirmation_type);
          return;
        }

        NotifyDidProcessDeposit(transaction);

        NotifyStatementOfAccountsDidChange();

        confirmations_->Confirm(transaction);
      });
}

void Account::ProcessUnclearedTransactions() {
  const WalletInfo& wallet = GetWallet();
  redeem_unblinded_payment_tokens_->MaybeRedeemAfterDelay(wallet);
}

void Account::TopUpUnblindedTokens() {
  if (!ShouldRewardUser()) {
    return;
  }

  const WalletInfo& wallet = GetWallet();
  refill_unblinded_tokens_->MaybeRefill(wallet);
}

void Account::Reset() {
  ResetRewards([=](const bool success) {
    if (!success) {
      BLOG(0, "Failed to reset rewards state");
      return;
    }

    BLOG(3, "Successfully reset rewards state");

    NotifyStatementOfAccountsDidChange();
  });
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

void Account::OnDidConfirm(const ConfirmationInfo& confirmation) {
  DCHECK(confirmation.IsValid());

  TopUpUnblindedTokens();
}

void Account::OnFailedToConfirm(const ConfirmationInfo& confirmation) {
  DCHECK(confirmation.IsValid());

  TopUpUnblindedTokens();
}

void Account::OnDidGetIssuers(const IssuersInfo& issuers) {
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

void Account::OnFailedToGetIssuers() {
  BLOG(0, "Failed to get issuers");
}

void Account::OnDidRedeemUnblindedPaymentTokens(
    const privacy::UnblindedPaymentTokenList& unblinded_payment_tokens) {
  BLOG(1, "Successfully redeemed unblinded payment tokens");

  database::table::Transactions database_table;
  database_table.Update(unblinded_payment_tokens, [](const bool success) {
    if (!success) {
      BLOG(0, "Failed to update transactions");
      return;
    }

    BLOG(3, "Successfully updated transactions");
  });
}

void Account::OnFailedToRedeemUnblindedPaymentTokens() {
  BLOG(1, "Failed to redeem unblinded payment tokens");
}

void Account::OnDidRetryRedeemingUnblindedPaymentTokens() {
  BLOG(1, "Retry redeeming unblinded payment tokens");
}

void Account::OnDidRefillUnblindedTokens() {
  BLOG(1, "Successfully refilled unblinded tokens");

  AdsClientHelper::Get()->ClearScheduledCaptcha();
}

void Account::OnCaptchaRequiredToRefillUnblindedTokens(
    const std::string& captcha_id) {
  BLOG(1, "Captcha required to refill unblinded tokens");

  const WalletInfo& wallet = GetWallet();
  AdsClientHelper::Get()->ShowScheduledCaptchaNotification(wallet.id,
                                                           captcha_id);
}

}  // namespace ads
