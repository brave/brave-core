// Copyright 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation
import BraveCore
import OrderedCollections

extension BraveWalletKeyringService {
  
  /// Check if any wallet account has been created given a coin type and a chain id
  @MainActor func isAccountAvailable(for coin: BraveWallet.CoinType, chainId: String) async -> Bool {
    // KeyringId must be checked with chainId for Filecoin, BTC (2 keyring types).
    let keyringId = BraveWallet.KeyringId.keyringId(for: coin, on: chainId)
    // `KeyringInfo.isKeyringCreated` insufficient check as this value can
    // be true with no accounts after wallet restore.
    let allAccountsForKeyring = await allAccounts().accounts.filter { $0.keyringId == keyringId }
    return !allAccountsForKeyring.isEmpty
  }
}
