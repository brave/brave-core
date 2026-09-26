/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_POLKADOT_POLKADOT_DAPP_UTILS_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_POLKADOT_POLKADOT_DAPP_UTILS_H_

#include <optional>
#include <string>
#include <vector>

#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"

namespace brave_wallet {

class KeyringService;

// Identifiers of every Polkadot account, in the form the permission layer
// stores them. See `GetAccountPermissionIdentifier`.
std::vector<std::string> GetPolkadotAccountPermissionIdentifiers(
    KeyringService* keyring_service);

// The Polkadot account a dapp should be talking to, out of `allowed_accounts`.
// `allowed_accounts` is passed in rather than queried from the delegate so
// callers can use the list a permission request just resolved with, before the
// iOS front-end database has necessarily finished writing it.
mojom::AccountIdPtr GetPolkadotPreferredDappAccount(
    KeyringService* keyring_service,
    const std::optional<std::vector<std::string>>& allowed_accounts);

// Converts an account into the `InjectedAccount` shape dapps expect.
mojom::PolkadotInjectedAccountPtr MakePolkadotInjectedAccount(
    const mojom::AccountInfo& account);

}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_POLKADOT_POLKADOT_DAPP_UTILS_H_
