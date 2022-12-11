/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_VIEWS_TOOLBAR_WALLET_BUTTON_NOTIFICATION_SOURCE_H_
#define BRAVE_BROWSER_UI_VIEWS_TOOLBAR_WALLET_BUTTON_NOTIFICATION_SOURCE_H_

#include <memory>
#include <string>

#include "base/memory/weak_ptr.h"
#include "brave/components/brave_wallet/browser/keyring_service.h"
#include "brave/components/brave_wallet/browser/keyring_service_observer_base.h"
#include "brave/components/brave_wallet/browser/tx_service.h"
#include "brave/components/brave_wallet/browser/tx_status_resolver.h"
#include "chrome/browser/profiles/profile.h"
#include "components/prefs/pref_service.h"
#include "third_party/abseil-cpp/absl/types/optional.h"

namespace brave {

using WalletButtonNotificationSourceCallback =
    base::RepeatingCallback<void(bool /* show suggest */,
                                 size_t /* counter */)>;

class WalletButtonNotificationSource
    : brave_wallet::mojom::TxServiceObserver,
      brave_wallet::KeyringServiceObserverBase {
 public:
  WalletButtonNotificationSource(
      Profile* profile,
      WalletButtonNotificationSourceCallback callback);
  ~WalletButtonNotificationSource() override;

  void MarkWalletButtonWasClicked();

 private:
  // brave_wallet::mojom::TxServiceObserver
  void OnNewUnapprovedTx(
      brave_wallet::mojom::TransactionInfoPtr tx_info) override;
  void OnUnapprovedTxUpdated(
      brave_wallet::mojom::TransactionInfoPtr tx_info) override;
  void OnTransactionStatusChanged(
      brave_wallet::mojom::TransactionInfoPtr tx_info) override;
  void OnTxServiceReset() override;

  // brave_wallet::KeyringServiceObserverBase
  void KeyringCreated(const std::string& keyring_id) override;

  void CheckTxStatus();
  void OnTxStatusResolved(size_t count);
  void OnKeyringInfoResolved(brave_wallet::mojom::KeyringInfoPtr keyring_info);

  void NotifyObservers();

  raw_ptr<PrefService> prefs_ = nullptr;
  raw_ptr<brave_wallet::TxService> tx_service_ = nullptr;
  raw_ptr<brave_wallet::KeyringService> keyring_service_ = nullptr;

  mojo::Receiver<brave_wallet::mojom::TxServiceObserver> tx_observer_{this};
  mojo::Receiver<brave_wallet::mojom::KeyringServiceObserver>
      keyring_service_observer_{this};

  std::unique_ptr<brave_wallet::TxStatusResolver> status_resolver_;

  WalletButtonNotificationSourceCallback callback_;

  absl::optional<bool> wallet_created_ = false;
  size_t running_tx_count_ = 0;

  base::WeakPtrFactory<WalletButtonNotificationSource> weak_ptr_factory_{this};
};

}  // namespace brave

#endif  // BRAVE_BROWSER_UI_VIEWS_TOOLBAR_WALLET_BUTTON_NOTIFICATION_SOURCE_H_
