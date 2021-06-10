/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/account/account.h"

#include "bat/ads/internal/account/ad_rewards/ad_rewards.h"
#include "bat/ads/internal/account/confirmations/confirmation_info.h"
#include "bat/ads/internal/account/confirmations/confirmations.h"
#include "bat/ads/internal/account/confirmations/confirmations_state.h"
#include "bat/ads/internal/account/statement/statement.h"
#include "bat/ads/internal/account/transactions/transactions.h"
#include "bat/ads/internal/account/wallet/wallet.h"
#include "bat/ads/internal/account/wallet/wallet_info.h"
#include "bat/ads/internal/logging.h"
#include "bat/ads/internal/privacy/tokens/token_generator_interface.h"
#include "bat/ads/internal/privacy/unblinded_tokens/unblinded_tokens.h"
#include "bat/ads/internal/tokens/redeem_unblinded_payment_tokens/redeem_unblinded_payment_tokens.h"
#include "bat/ads/internal/tokens/refill_unblinded_tokens/refill_unblinded_tokens.h"

namespace ads {

Account::Account(privacy::TokenGeneratorInterface* token_generator)
    : token_generator_(token_generator),
      ad_rewards_(std::make_unique<AdRewards>()),
      confirmations_(
          std::make_unique<Confirmations>(token_generator, ad_rewards_.get())),
      redeem_unblinded_payment_tokens_(
          std::make_unique<RedeemUnblindedPaymentTokens>()),
      refill_unblinded_tokens_(
          std::make_unique<RefillUnblindedTokens>(token_generator)),
      statement_(std::make_unique<Statement>(ad_rewards_.get())),
      wallet_(std::make_unique<Wallet>()) {
  DCHECK(token_generator_);

  confirmations_->AddObserver(this);

  ad_rewards_->set_delegate(this);
  redeem_unblinded_payment_tokens_->set_delegate(this);
}

Account::~Account() {
  confirmations_->RemoveObserver(this);
  ad_rewards_->set_delegate(nullptr);
  redeem_unblinded_payment_tokens_->set_delegate(nullptr);
}

void Account::AddObserver(AccountObserver* observer) {
  DCHECK(observer);
  observers_.AddObserver(observer);
}

void Account::RemoveObserver(AccountObserver* observer) {
  DCHECK(observer);
  observers_.RemoveObserver(observer);
}

bool Account::SetWallet(const std::string& id, const std::string& seed) {
  const WalletInfo last_wallet = wallet_->Get();

  if (!wallet_->Set(id, seed)) {
    BLOG(0, "Invalid wallet");
    return false;
  }

  const WalletInfo wallet = wallet_->Get();

  if (last_wallet.IsValid() && last_wallet != wallet) {
    NotifyWalletRestored(wallet);
  }

  NotifyWalletChanged(wallet);

  return true;
}

WalletInfo Account::GetWallet() const {
  return wallet_->Get();
}

void Account::SetCatalogIssuers(const CatalogIssuersInfo& catalog_issuers) {
  confirmations_->SetCatalogIssuers(catalog_issuers);
}

void Account::Deposit(const std::string& creative_instance_id,
                      const ConfirmationType& confirmation_type) {
  confirmations_->ConfirmAd(creative_instance_id, confirmation_type);
}

StatementInfo Account::GetStatement(const int64_t from_timestamp,
                                    const int64_t to_timestamp) const {
  DCHECK(to_timestamp >= from_timestamp);
  return statement_->Get(from_timestamp, to_timestamp);
}

void Account::Reconcile() {
  const WalletInfo wallet = GetWallet();
  ad_rewards_->MaybeReconcile(wallet);
}

void Account::ProcessTransactions() {
  confirmations_->RetryAfterDelay();

  ProcessUnclearedTransactions();
}

void Account::TopUpUnblindedTokens() {
  const WalletInfo wallet = GetWallet();
  refill_unblinded_tokens_->MaybeRefill(wallet);
}

///////////////////////////////////////////////////////////////////////////////

void Account::ProcessUnclearedTransactions() {
  const WalletInfo wallet = GetWallet();
  redeem_unblinded_payment_tokens_->MaybeRedeemAfterDelay(wallet);
}

void Account::NotifyWalletChanged(const WalletInfo& wallet) const {
  for (AccountObserver& observer : observers_) {
    observer.OnWalletChanged(wallet);
  }
}

void Account::NotifyWalletRestored(const WalletInfo& wallet) const {
  for (AccountObserver& observer : observers_) {
    observer.OnWalletRestored(wallet);
  }
}

void Account::NotifyWalletInvalid() const {
  for (AccountObserver& observer : observers_) {
    observer.OnWalletInvalid();
  }
}

void Account::NotifyCatalogIssuersChanged(
    const CatalogIssuersInfo& catalog_issuers) const {
  for (AccountObserver& observer : observers_) {
    observer.OnCatalogIssuersChanged(catalog_issuers);
  }
}

void Account::NotifyAdRewardsChanged() const {
  for (AccountObserver& observer : observers_) {
    observer.OnAdRewardsChanged();
  }
}

void Account::NotifyTransactionsChanged() const {
  for (AccountObserver& observer : observers_) {
    observer.OnTransactionsChanged();
  }
}

void Account::NotifyUnclearedTransactionsProcessed() const {
  for (AccountObserver& observer : observers_) {
    observer.OnUnclearedTransactionsProcessed();
  }
}

void Account::OnConfirmAd(const double estimated_redemption_value,
                          const ConfirmationInfo& confirmation) {
  transactions::Add(estimated_redemption_value, confirmation);
  NotifyTransactionsChanged();

  TopUpUnblindedTokens();
}

void Account::OnConfirmAdFailed(const ConfirmationInfo& confirmation) {
  TopUpUnblindedTokens();

  confirmations_->RetryAfterDelay();
}

void Account::OnDidReconcileAdRewards() {
  NotifyAdRewardsChanged();
}

void Account::OnDidRedeemUnblindedPaymentTokens() {
  BLOG(1, "Successfully redeemed unblinded payment tokens");

  if (ConfirmationsState::Get()->get_unblinded_payment_tokens()->IsEmpty()) {
    return;
  }

  const TransactionList transactions = transactions::GetUncleared();
  ad_rewards_->SetUnreconciledTransactions(transactions);

  ConfirmationsState::Get()->get_unblinded_payment_tokens()->RemoveAllTokens();
  ConfirmationsState::Get()->Save();

  Reconcile();
}

void Account::OnFailedToRedeemUnblindedPaymentTokens() {
  BLOG(1, "Failed to redeem unblinded payment tokens");
}

void Account::OnDidRetryRedeemingUnblindedPaymentTokens() {
  BLOG(1, "Retry redeeming unblinded payment tokens");
}

}  // namespace ads
