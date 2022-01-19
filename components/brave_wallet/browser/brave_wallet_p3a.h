/* Copyright 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_BRAVE_WALLET_P3A_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_BRAVE_WALLET_P3A_H_

#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"
#include "mojo/public/cpp/bindings/receiver.h"

class PrefService;

namespace brave_wallet {

class BraveWalletService;
class KeyringService;

// Reports BraveWallet related P3A data.
// Maintains a timer to report in the amount of up time.
class BraveWalletP3A : public mojom::BraveWalletServiceObserver,
                       public mojom::KeyringServiceObserver {
 public:
  BraveWalletP3A(BraveWalletService* wallet_service,
                 KeyringService* keyring_service,
                 PrefService* pref_service);

  ~BraveWalletP3A() override;
  BraveWalletP3A(const BraveWalletP3A&) = delete;
  BraveWalletP3A& operator=(BraveWalletP3A&) = delete;

  // KeyringServiceObserver
  void KeyringCreated() override;
  void KeyringRestored() override;
  void KeyringReset() override;
  void Locked() override;
  void Unlocked() override;
  void BackedUp() override;
  void AccountsChanged() override;
  void AutoLockMinutesChanged() override;
  void SelectedAccountChanged() override;

  // BraveWalletServiceObserver
  void OnActiveOriginChanged(const std::string& origin) override;
  void OnDefaultWalletChanged(
      brave_wallet::mojom::DefaultWallet wallet) override;
  void OnDefaultBaseCurrencyChanged(const std::string& currency) override;
  void OnDefaultBaseCryptocurrencyChanged(
      const std::string& cryptocurrency) override;
  void OnNetworkListChanged() override;

 private:
  void RecordInitialBraveWalletP3AState();
  BraveWalletService* wallet_service_;
  KeyringService* keyring_service_;
  PrefService* pref_service_ = nullptr;

  mojo::Receiver<brave_wallet::mojom::BraveWalletServiceObserver>
      wallet_service_observer_receiver_{this};
  mojo::Receiver<brave_wallet::mojom::KeyringServiceObserver>
      keyring_service_observer_receiver_{this};
};

}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_BRAVE_WALLET_P3A_H_
