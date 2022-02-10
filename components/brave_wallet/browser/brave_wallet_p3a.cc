/* Copyright 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/brave_wallet_p3a.h"

#include "base/metrics/histogram_macros.h"
#include "brave/components/brave_wallet/browser/brave_wallet_service.h"
#include "brave/components/brave_wallet/browser/keyring_service.h"
#include "brave/components/brave_wallet/browser/pref_names.h"
#include "components/prefs/pref_service.h"

namespace brave_wallet {

// Has the Wallet keyring been created?
// i) No, ii) Yes
void RecordKeyringCreated(mojom::KeyringInfoPtr keyring_info) {
  UMA_HISTOGRAM_BOOLEAN(
      "Brave.Wallet.KeyringCreated",
      static_cast<int>(keyring_info->is_default_keyring_created));
}

// What is the DefaultWalletSetting?
// i) AskDeprecated, ii) None, ii) CryptoWallets,
// iv) BraveWalletPreferExtension, v) BraveWallet
void RecordDefaultWalletSetting(PrefService* pref_service) {
  const int max_bucket =
      static_cast<int>(brave_wallet::mojom::DefaultWallet::kMaxValue);
  auto default_wallet = pref_service->GetInteger(kDefaultWallet2);
  UMA_HISTOGRAM_EXACT_LINEAR("Brave.Wallet.DefaultWalletSetting",
                             default_wallet, max_bucket);
}

BraveWalletP3A::BraveWalletP3A(BraveWalletService* wallet_service,
                               KeyringService* keyring_service,
                               PrefService* pref_service)
    : wallet_service_(wallet_service),
      keyring_service_(keyring_service),
      pref_service_(pref_service) {
  RecordInitialBraveWalletP3AState();
  wallet_service_->AddObserver(
      wallet_service_observer_receiver_.BindNewPipeAndPassRemote());
  keyring_service_->AddObserver(
      keyring_service_observer_receiver_.BindNewPipeAndPassRemote());
}

BraveWalletP3A::~BraveWalletP3A() = default;

void BraveWalletP3A::RecordInitialBraveWalletP3AState() {
  keyring_service_->GetKeyringInfo(mojom::kDefaultKeyringId,
                                   base::BindOnce(&RecordKeyringCreated));
  RecordDefaultWalletSetting(pref_service_);
}

// KeyringServiceObserver
void BraveWalletP3A::KeyringCreated() {
  keyring_service_->GetKeyringInfo(mojom::kDefaultKeyringId,
                                   base::BindOnce(&RecordKeyringCreated));
}

// BraveWalletServiceObserver
void BraveWalletP3A::OnDefaultWalletChanged(
    brave_wallet::mojom::DefaultWallet default_wallet) {
  RecordDefaultWalletSetting(pref_service_);
}

}  // namespace brave_wallet
