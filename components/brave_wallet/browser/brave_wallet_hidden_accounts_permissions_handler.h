/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_BRAVE_WALLET_HIDDEN_ACCOUNTS_PERMISSIONS_HANDLER_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_BRAVE_WALLET_HIDDEN_ACCOUNTS_PERMISSIONS_HANDLER_H_

#include <string>
#include <vector>

#include "base/containers/flat_set.h"
#include "base/memory/raw_ref.h"
#include "base/memory/weak_ptr.h"
#include "brave/components/brave_wallet/browser/keyring_service_observer_base.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"
#include "mojo/public/cpp/bindings/receiver.h"

namespace brave_wallet {

class BraveWalletServiceDelegate;
class KeyringService;

class BraveWalletHiddenAccountsPermissionsHandler
    : public KeyringServiceObserverBase {
 public:
  BraveWalletHiddenAccountsPermissionsHandler(
      KeyringService& keyring_service,
      BraveWalletServiceDelegate& delegate);
  BraveWalletHiddenAccountsPermissionsHandler(
      const BraveWalletHiddenAccountsPermissionsHandler&) = delete;
  BraveWalletHiddenAccountsPermissionsHandler& operator=(
      const BraveWalletHiddenAccountsPermissionsHandler&) = delete;
  ~BraveWalletHiddenAccountsPermissionsHandler() override;

  // KeyringServiceObserverBase:
  void AccountsChanged() override;

 private:
  void ResetPermissionsForHiddenAccounts(
      std::vector<mojom::AccountInfoPtr> hidden_accounts);
  void ResolveWebsitePermissionsForHiddenAccount(
      mojom::CoinType coin,
      std::string hidden_account_identifier,
      const std::vector<std::string>& websites);

  raw_ref<KeyringService> keyring_service_;
  raw_ref<BraveWalletServiceDelegate> delegate_;
  base::flat_set<std::string> hidden_account_identifiers_;
  mojo::Receiver<brave_wallet::mojom::KeyringServiceObserver>
      keyring_service_observer_receiver_{this};
  base::WeakPtrFactory<BraveWalletHiddenAccountsPermissionsHandler>
      weak_ptr_factory_{this};
};

}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_BRAVE_WALLET_HIDDEN_ACCOUNTS_PERMISSIONS_HANDLER_H_
