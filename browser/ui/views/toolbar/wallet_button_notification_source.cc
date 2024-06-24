/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/toolbar/wallet_button_notification_source.h"

#include <utility>

#include "brave/browser/brave_wallet/brave_wallet_service_factory.h"
#include "brave/components/brave_wallet/browser/brave_wallet_service.h"
#include "brave/components/brave_wallet/browser/keyring_service.h"
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
  // Already connected.
  if (tx_observer_.is_bound()) {
    return;
  }
  auto* brave_wallet_service =
      brave_wallet::BraveWalletServiceFactory::GetServiceForContext(profile_);
  if (!brave_wallet_service) {
    return;
  }

  tx_service_ = brave_wallet_service->tx_service();
  tx_service_->AddObserver(tx_observer_.BindNewPipeAndPassRemote());
  CheckTxStatus();
}

void WalletButtonNotificationSource::EnsureKeyringServiceConnected() {
  // Already connected.
  if (keyring_service_observer_.is_bound()) {
    return;
  }

  auto* brave_wallet_service =
      brave_wallet::BraveWalletServiceFactory::GetServiceForContext(profile_);
  if (!brave_wallet_service) {
    return;
  }

  auto* keyring_service = brave_wallet_service->keyring_service();

  keyring_service->AddObserver(
      keyring_service_observer_.BindNewPipeAndPassRemote());

  wallet_created_ = keyring_service->IsWalletCreatedSync();
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
