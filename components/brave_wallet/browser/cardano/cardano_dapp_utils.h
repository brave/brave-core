/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_CARDANO_CARDANO_DAPP_UTILS_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_CARDANO_CARDANO_DAPP_UTILS_H_

#include <string>
#include <vector>

#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"

namespace brave_wallet {

class BraveWalletProviderDelegate;
class KeyringService;

std::vector<std::string> GetCardanoAccountPermissionIdentifiers(
    KeyringService* keyring_service);

mojom::AccountIdPtr GetCardanoPreferredDappAccount(
    BraveWalletProviderDelegate* delegate,
    KeyringService* keyring_service);

}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_CARDANO_CARDANO_DAPP_UTILS_H_
