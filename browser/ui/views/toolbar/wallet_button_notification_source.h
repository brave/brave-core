/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_VIEWS_TOOLBAR_WALLET_BUTTON_NOTIFICATION_SOURCE_H_
#define BRAVE_BROWSER_UI_VIEWS_TOOLBAR_WALLET_BUTTON_NOTIFICATION_SOURCE_H_

#include <optional>

#include "base/memory/weak_ptr.h"
#include "brave/components/brave_wallet/browser/keyring_service_observer_base.h"
#include "brave/components/brave_wallet/browser/tx_service.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"
#include "chrome/browser/profiles/profile.h"
#include "components/prefs/pref_service.h"
#include "mojo/public/cpp/bindings/receiver.h"

namespace brave_wallet {

using WalletButtonNotificationSourceCallback =
    base::RepeatingCallback<void(bool /* show suggest */,
                                 size_t /* counter */)>;

// Provides and updates data for the wallet button notification badge.
// Like number of pending transactions or onboarding bubble to show.
class WalletButtonNotificationSource : mojom::TxServiceObserver,
                                       KeyringServiceObserverBase {
 public:
  WalletButtonNotificationSource(
      Profile* profile,
      WalletButtonNotificationSourceCallback callback);
  ~WalletButtonNotificationSource() override;

  void MarkWalletButtonWasClicked();
  void Init();

 private:
  void EnsureTxServiceConnected();

  void EnsureKeyringServiceConnected();

  // mojom::TxServiceObserver
  void OnNewUnapprovedTx(mojom::TransactionInfoPtr tx_info) override;
  void OnUnapprovedTxUpdated(mojom::TransactionInfoPtr tx_info) override {}
  void OnTransactionStatusChanged(mojom::TransactionInfoPtr tx_info) override;
  void OnTxServiceReset() override;

  // KeyringServiceObserverBase
  void WalletCreated() override;
  void WalletRestored() override;

  void OnWalletReady();
  void CheckTxStatus();

  void NotifyObservers();

  raw_ptr<Profile> profile_ = nullptr;
  raw_ptr<PrefService> prefs_ = nullptr;
  raw_ptr<TxService> tx_service_ = nullptr;

  mojo::Receiver<mojom::TxServiceObserver> tx_observer_{this};
  mojo::Receiver<mojom::KeyringServiceObserver> keyring_service_observer_{this};

  WalletButtonNotificationSourceCallback callback_;

  std::optional<bool> wallet_created_;
  uint32_t pending_tx_count_ = 0;

  base::WeakPtrFactory<WalletButtonNotificationSource> weak_ptr_factory_{this};
};

}  // namespace brave_wallet

#endif  // BRAVE_BROWSER_UI_VIEWS_TOOLBAR_WALLET_BUTTON_NOTIFICATION_SOURCE_H_
