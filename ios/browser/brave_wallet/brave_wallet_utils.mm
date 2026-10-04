// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/ios/browser/brave_wallet/brave_wallet_utils.h"

#include "brave/components/brave_wallet/browser/brave_wallet_utils.h"
#include "components/prefs/pref_service.h"

namespace brave_wallet {

namespace {

bool IsDefaultBrave(mojom::DefaultWallet wallet) {
  // iOS does not have a separate extension so we consider Brave the default
  // wallet for either state.
  return wallet == mojom::DefaultWallet::BraveWallet ||
         wallet == mojom::DefaultWallet::BraveWalletPreferExtension;
}

}  // namespace

bool IsDefaultEthereumWalletBrave(PrefService* prefs) {
  return IsDefaultBrave(GetDefaultEthereumWallet(prefs));
}

bool IsDefaultCardanoWalletBrave(PrefService* prefs) {
  return IsDefaultBrave(GetDefaultCardanoWallet(prefs));
}

}  // namespace brave_wallet
