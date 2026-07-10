// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/ios/browser/brave_wallet/brave_wallet_utils.h"

#include "brave/components/brave_wallet/browser/brave_wallet_utils.h"
#include "components/prefs/pref_service.h"

namespace brave_wallet {

bool IsDefaultEthereumWalletBrave(PrefService* prefs) {
  mojom::DefaultWallet eth_wallet = GetDefaultEthereumWallet(prefs);
  // iOS does not have a separate extension so we consider Brave the default
  // wallet for either state.
  return eth_wallet == mojom::DefaultWallet::BraveWallet ||
         eth_wallet == mojom::DefaultWallet::BraveWalletPreferExtension;
}

}  // namespace brave_wallet
