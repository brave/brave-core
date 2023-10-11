/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_VIEWS_TOOLBAR_WALLET_BUTTON_NOTIFICATION_SOURCE_H_
#define BRAVE_BROWSER_UI_VIEWS_TOOLBAR_WALLET_BUTTON_NOTIFICATION_SOURCE_H_

#include "base/memory/weak_ptr.h"
#include "brave/components/brave_wallet/browser/keyring_service.h"
#include "brave/components/brave_wallet/browser/keyring_service_observer_base.h"
#include "brave/components/brave_wallet/browser/tx_service.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"
#include "chrome/browser/profiles/profile.h"
#include "components/prefs/pref_service.h"
#include "mojo/public/cpp/bindings/receiver.h"
#include "third_party/abseil-cpp/absl/types/optional.h"

namespace brave {

using WalletButtonNotificationSourceCallback =
    base::RepeatingCallback<void(bool /* show suggest */,
                                 size_t /* counter */)>;

// Provides and updates data for the wallet button notification badge.
// Like number of pending transactions or onboarding bubble to show.
class WalletButtonNotificationSource
    : brave_wallet::mojom::TxServiceObserver,
      brave_wallet::KeyringServiceObserverBase {
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

  // brave_wallet::mojom::TxServiceObserver
  void OnNewUnapprovedTx(
      brave_wallet::mojom::TransactionInfoPtr tx_info) override;
  void OnUnapprovedTxUpdated(
      brave_wallet::mojom::TransactionInfoPtr tx_info) override {}
  void OnTransactionStatusChanged(
      brave_wallet::mojom::TransactionInfoPtr tx_info) override;
  void OnTxServiceReset() override;

  // brave_wallet::KeyringServiceObserverBase
  void WalletCreated() override;
  void WalletRestored() override;

  void OnWalletReady();
  void CheckTxStatus();

  void NotifyObservers();

  raw_ptr<Profile> profile_ = nullptr;
  raw_ptr<PrefService> prefs_ = nullptr;
  raw_ptr<brave_wallet::TxService> tx_service_ = nullptr;
  raw_ptr<brave_wallet::KeyringService> keyring_service_ = nullptr;

  mojo::Receiver<brave_wallet::mojom::TxServiceObserver> tx_observer_{this};
  mojo::Receiver<brave_wallet::mojom::KeyringServiceObserver>
      keyring_service_observer_{this};

  WalletButtonNotificationSourceCallback callback_;

  absl::optional<bool> wallet_created_;
  uint32_t pending_tx_count_ = 0;

  base::WeakPtrFactory<WalletButtonNotificationSource> weak_ptr_factory_{this};
};

}  // namespace brave

#endif  // BRAVE_BROWSER_UI_VIEWS_TOOLBAR_WALLET_BUTTON_NOTIFICATION_SOURCE_H_
