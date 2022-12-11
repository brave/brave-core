/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/toolbar/wallet_button_notification_source.h"

#include "brave/browser/brave_wallet/keyring_service_factory.h"
#include "brave/browser/brave_wallet/tx_service_factory.h"
#include "brave/components/brave_wallet/browser/brave_wallet_utils.h"
#include "brave/components/brave_wallet/browser/pref_names.h"

namespace brave {

WalletButtonNotificationSource::WalletButtonNotificationSource(
    Profile* profile,
    WalletButtonNotificationSourceCallback callback)
    : callback_(callback) {
  prefs_ = profile->GetPrefs();
  tx_service_ = brave_wallet::TxServiceFactory::GetServiceForContext(profile);
  keyring_service_ =
      brave_wallet::KeyringServiceFactory::GetServiceForContext(profile);
  tx_service_->AddObserver(tx_observer_.BindNewPipeAndPassRemote());
  keyring_service_->AddObserver(
      keyring_service_observer_.BindNewPipeAndPassRemote());
  CheckTxStatus();
}

WalletButtonNotificationSource::~WalletButtonNotificationSource() {}

void WalletButtonNotificationSource::MarkWalletButtonWasClicked() {
  prefs_->SetBoolean(kWalletButtonClicked, true);
  NotifyObservers();
}

void WalletButtonNotificationSource::CheckTxStatus() {
  if (!tx_service_) {
    NOTREACHED();
    return;
  }
  status_resolver_ =
      std::make_unique<brave_wallet::TxStatusResolver>(tx_service_);
  status_resolver_->GetPendingTransactionsCount(
      base::BindOnce(&WalletButtonNotificationSource::OnTxStatusResolved,
                     weak_ptr_factory_.GetWeakPtr()));
}

void WalletButtonNotificationSource::OnUnapprovedTxUpdated(
    brave_wallet::mojom::TransactionInfoPtr tx_info) {
  CheckTxStatus();
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
  OnTxStatusResolved(0u);
}

void WalletButtonNotificationSource::OnTxStatusResolved(size_t count) {
  running_tx_count_ = count;
  status_resolver_.reset();

  if (prefs_->GetBoolean(kWalletButtonClicked)) {
    NotifyObservers();
    return;
  }
  keyring_service_->GetKeyringInfo(
      brave_wallet::mojom::kDefaultKeyringId,
      base::BindOnce(&WalletButtonNotificationSource::OnKeyringInfoResolved,
                     weak_ptr_factory_.GetWeakPtr()));
}

void WalletButtonNotificationSource::KeyringCreated(
    const std::string& keyring_id) {
  if (keyring_id == brave_wallet::mojom::kDefaultKeyringId) {
    prefs_->SetBoolean(kWalletButtonClicked, true);
    NotifyObservers();
  }
}

void WalletButtonNotificationSource::NotifyObservers() {
  bool show_suggestion_badge =
      (wallet_created_.has_value() && !wallet_created_.value() &&
       !prefs_->GetBoolean(kWalletButtonClicked));
  callback_.Run(show_suggestion_badge, running_tx_count_);
}

void WalletButtonNotificationSource::OnKeyringInfoResolved(
    brave_wallet::mojom::KeyringInfoPtr keyring_info) {
  wallet_created_ = keyring_info && keyring_info->is_keyring_created;
  if (wallet_created_.value()) {
    prefs_->SetBoolean(kWalletButtonClicked, true);
  }
  NotifyObservers();
}

}  // namespace brave
