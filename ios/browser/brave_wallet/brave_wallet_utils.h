// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_IOS_BROWSER_BRAVE_WALLET_BRAVE_WALLET_UTILS_H_
#define BRAVE_IOS_BROWSER_BRAVE_WALLET_BRAVE_WALLET_UTILS_H_

class PrefService;

namespace brave_wallet {

// Whether or not the default ethereum wallet is handled by Brave
bool IsDefaultEthereumWalletBrave(PrefService* prefs);

}  // namespace brave_wallet

#endif  // BRAVE_IOS_BROWSER_BRAVE_WALLET_BRAVE_WALLET_UTILS_H_
