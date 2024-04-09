// Copyright 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveCore
import Foundation
import OrderedCollections
import Preferences

extension BraveWalletKeyringService {

  /// Check if any wallet account has been created given a coin type and a chain id
  @MainActor func isAccountAvailable(for coin: BraveWallet.CoinType, chainId: String) async -> Bool
  {
    // KeyringId must be checked with chainId for Filecoin, BTC (2 keyring types).
    let keyringId = BraveWallet.KeyringId.keyringId(for: coin, on: chainId)
    // `KeyringInfo.isKeyringCreated` insufficient check as this value can
    // be true with no accounts after wallet restore.
    let allAccountsForKeyring = await allAccounts().accounts.filter { $0.keyringId == keyringId }
    return !allAccountsForKeyring.isEmpty
  }

  /// Return a list of all accounts with checking if Bitcoin testnet is enabled
  /// The list of account will not include Bitcoin Testnet Accounts if Bitcoin testnet is disabled.
  func allAccountInfos(checkBTCTestnet: Bool = true) async -> [BraveWallet.AccountInfo] {
    var accounts = await self.allAccounts().accounts
    if checkBTCTestnet, !Preferences.Wallet.isBitcoinTestnetEnabled.value {
      accounts = accounts.filter({ $0.keyringId != BraveWallet.KeyringId.bitcoin84Testnet })
    }
    return accounts
  }
}
