/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/toolbar/wallet_button_notification_source.h"

#include <utility>

#include "brave/browser/brave_wallet/keyring_service_factory.h"
#include "brave/browser/brave_wallet/tx_service_factory.h"
#include "brave/components/brave_wallet/browser/pref_names.h"

namespace brave {

WalletButtonNotificationSource::WalletButtonNotificationSource(
    Profile* profile,
    WalletButtonNotificationSourceCallback callback)
    : profile_(profile), callback_(callback) {
  prefs_ = profile->GetPrefs();
}

void WalletButtonNotificationSource::Init() {
  EnsureKeyringServiceConnected();
  EnsureTxServiceConnected();
}

void WalletButtonNotificationSource::EnsureTxServiceConnected() {
  tx_service_ = brave_wallet::TxServiceFactory::GetServiceForContext(profile_);
  if (!tx_service_) {
    return;
  }
  tx_service_->AddObserver(tx_observer_.BindNewPipeAndPassRemote());
  CheckTxStatus();
}

void WalletButtonNotificationSource::EnsureKeyringServiceConnected() {
  keyring_service_ =
      brave_wallet::KeyringServiceFactory::GetServiceForContext(profile_);
  if (!keyring_service_) {
    return;
  }

  keyring_service_->AddObserver(
      keyring_service_observer_.BindNewPipeAndPassRemote());

  wallet_created_ = keyring_service_->IsWalletSetup();
  if (wallet_created_.value()) {
    prefs_->SetBoolean(kShouldShowWalletSuggestionBadge, false);
  }
  NotifyObservers();
}

WalletButtonNotificationSource::~WalletButtonNotificationSource() = default;

void WalletButtonNotificationSource::MarkWalletButtonWasClicked() {
  prefs_->SetBoolean(kShouldShowWalletSuggestionBadge, false);
  NotifyObservers();
}

void WalletButtonNotificationSource::CheckTxStatus() {
  if (!tx_service_) {
    return;
  }
  pending_tx_count_ = tx_service_->GetPendingTransactionsCountSync();
  NotifyObservers();
}

void WalletButtonNotificationSource::OnTransactionStatusChanged(
    brave_wallet::mojom::TransactionInfoPtr tx_info) {
  CheckTxStatus();
}

void WalletButtonNotificationSource::OnNewUnapprovedTx(
    brave_wallet::mojom::TransactionInfoPtr tx_info) {
  CheckTxStatus();
}

void WalletButtonNotificationSource::OnTxServiceReset() {
  pending_tx_count_ = 0;
  NotifyObservers();
}

void WalletButtonNotificationSource::OnWalletReady() {
  prefs_->SetBoolean(kShouldShowWalletSuggestionBadge, false);
  NotifyObservers();
}

void WalletButtonNotificationSource::WalletCreated() {
  OnWalletReady();
}

void WalletButtonNotificationSource::WalletRestored() {
  OnWalletReady();
}

void WalletButtonNotificationSource::NotifyObservers() {
  bool show_suggestion_badge =
      (wallet_created_.has_value() && !wallet_created_.value() &&
       prefs_->GetBoolean(kShouldShowWalletSuggestionBadge));
  callback_.Run(show_suggestion_badge, pending_tx_count_);
}

}  // namespace brave
