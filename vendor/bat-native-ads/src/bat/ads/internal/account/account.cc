/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/account/account.h"

#include "base/check.h"
#include "bat/ads/internal/account/ad_rewards/ad_rewards.h"
#include "bat/ads/internal/account/ad_rewards/ad_rewards_util.h"
#include "bat/ads/internal/account/confirmations/confirmation_info.h"
#include "bat/ads/internal/account/confirmations/confirmations.h"
#include "bat/ads/internal/account/statement/statement.h"
#include "bat/ads/internal/account/transactions/transactions.h"
#include "bat/ads/internal/account/wallet/wallet.h"
#include "bat/ads/internal/account/wallet/wallet_info.h"
#include "bat/ads/internal/ads_client_helper.h"
#include "bat/ads/internal/logging.h"
#include "bat/ads/internal/privacy/tokens/token_generator_interface.h"
#include "bat/ads/internal/privacy/unblinded_tokens/unblinded_tokens.h"
#include "bat/ads/internal/tokens/redeem_unblinded_payment_tokens/redeem_unblinded_payment_tokens.h"
#include "bat/ads/internal/tokens/refill_unblinded_tokens/refill_unblinded_tokens.h"
#include "bat/ads/statement_info.h"

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
  refill_unblinded_tokens_->set_delegate(this);
}

Account::~Account() {
  confirmations_->RemoveObserver(this);
  ad_rewards_->set_delegate(nullptr);
  redeem_unblinded_payment_tokens_->set_delegate(nullptr);
  refill_unblinded_tokens_->set_delegate(nullptr);
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
    NotifyInvalidWallet();
    return false;
  }

  const WalletInfo wallet = wallet_->Get();

  if (last_wallet.IsValid() && last_wallet != wallet) {
    ad_rewards_->Reset();

    NotifyWalletDidChange(wallet);
  }

  NotifyWalletDidUpdate(wallet);

  return true;
}

WalletInfo Account::GetWallet() const {
  return wallet_->Get();
}

void Account::SetCatalogIssuers(const CatalogIssuersInfo& catalog_issuers) {
  confirmations_->SetCatalogIssuers(catalog_issuers);
  NotifyCatalogIssuersDidChange(catalog_issuers);
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
  if (!ShouldRewardUser()) {
    return;
  }

  const WalletInfo wallet = GetWallet();
  ad_rewards_->MaybeReconcile(wallet);
}

void Account::ProcessTransactions() {
  if (!ShouldRewardUser()) {
    return;
  }

  confirmations_->RetryAfterDelay();

  ProcessUnclearedTransactions();
}

void Account::TopUpUnblindedTokens() {
  if (!ShouldRewardUser()) {
    return;
  }

  const WalletInfo wallet = GetWallet();
  refill_unblinded_tokens_->MaybeRefill(wallet);
}

///////////////////////////////////////////////////////////////////////////////

void Account::ProcessUnclearedTransactions() {
  const WalletInfo wallet = GetWallet();
  redeem_unblinded_payment_tokens_->MaybeRedeemAfterDelay(wallet);
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

void Account::NotifyCatalogIssuersDidChange(
    const CatalogIssuersInfo& catalog_issuers) const {
  for (AccountObserver& observer : observers_) {
    observer.OnCatalogIssuersDidChange(catalog_issuers);
  }
}

void Account::NotifyStatementOfAccountsDidChange() const {
  for (AccountObserver& observer : observers_) {
    observer.OnStatementOfAccountsDidChange();
  }
}

void Account::OnConfirmAd(const double estimated_redemption_value,
                          const ConfirmationInfo& confirmation) {
  transactions::Add(estimated_redemption_value, confirmation);
  NotifyStatementOfAccountsDidChange();

  TopUpUnblindedTokens();
}

void Account::OnConfirmAdFailed(const ConfirmationInfo& confirmation) {
  TopUpUnblindedTokens();

  confirmations_->RetryAfterDelay();
}

void Account::OnDidReconcileAdRewards() {
  NotifyStatementOfAccountsDidChange();
}

void Account::OnDidRedeemUnblindedPaymentTokens(
    const privacy::UnblindedTokenList unblinded_tokens) {
  BLOG(1, "Successfully redeemed unblinded payment tokens");

  const TransactionList transactions = transactions::GetUncleared();
  ad_rewards_->AppendUnreconciledTransactions(transactions);

  Reconcile();
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

  const WalletInfo wallet = GetWallet();
  AdsClientHelper::Get()->ShowScheduledCaptchaNotification(wallet.id,
                                                           captcha_id);
}

}  // namespace ads
