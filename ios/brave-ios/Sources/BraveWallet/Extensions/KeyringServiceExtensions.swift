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

  /// Check if any wallet account has been created given a coin type
  @MainActor func isAccountAvailable(for coin: BraveWallet.CoinType) async -> Bool {
    let allAccountsForCoin = await allAccounts().accounts.filter { $0.coin == coin }
    return !allAccountsForCoin.isEmpty
  }

  /// Return a list of all accounts with checking if Bitcoin/Zcash testnet is enabled
  /// The list of account will not include Bitcoin/Zcash Testnet Accounts if Bitcoin/Zcash testnet is disabled.
  func allAccountInfos(
    checkBTCTestnet: Bool = true,
    checkZcashTestnet: Bool = true
  ) async -> [BraveWallet.AccountInfo] {
    var accounts = await self.allAccounts().accounts
    if checkBTCTestnet, !Preferences.Wallet.isBitcoinTestnetEnabled.value {
      accounts = accounts.filter({ $0.keyringId != BraveWallet.KeyringId.bitcoin84Testnet })
    }
    if checkZcashTestnet, !Preferences.Wallet.isZcashTestnetEnabled.value {
      accounts = accounts.filter({ $0.keyringId != BraveWallet.KeyringId.zCashTestnet })
    }
    if FeatureList.kBraveWalletCardanoEnabled?.enabled == true,
      FeatureList.kBraveWalletWebUIIOS?.enabled != true
    {
      accounts = accounts.filter({
        $0.keyringId != BraveWallet.KeyringId.cardanoMainnet
          && $0.keyringId != BraveWallet.KeyringId.cardanoTestnet
      })
    }
    return accounts
  }
}
