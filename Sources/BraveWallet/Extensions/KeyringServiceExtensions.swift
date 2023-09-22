// Copyright 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation
import BraveCore
import OrderedCollections

extension BraveWalletKeyringService {
  
  // Fetches all keyrings for all given coin types
  func keyrings(
    for coins: OrderedSet<BraveWallet.CoinType>
  ) async -> [BraveWallet.KeyringInfo] {
    var allKeyrings: [BraveWallet.KeyringInfo] = []
    allKeyrings = await withTaskGroup(
      of: BraveWallet.KeyringInfo.self,
      returning: [BraveWallet.KeyringInfo].self,
      body: { @MainActor group in
        let keyringIds: [BraveWallet.KeyringId] = coins.flatMap(\.keyringIds)
        for keyringId in keyringIds {
          group.addTask { @MainActor in
            await self.keyringInfo(keyringId)
          }
        }
        return await group.reduce([BraveWallet.KeyringInfo](), { partialResult, prior in
          return partialResult + [prior]
        })
        .sorted(by: { lhs, rhs in
          if lhs.coin == .fil && rhs.coin == .fil {
            return lhs.id == BraveWallet.KeyringId.filecoin
          } else {
            return (lhs.coin ?? .eth).sortOrder < (rhs.coin ?? .eth).sortOrder
          }
        })
      }
    )
    return allKeyrings
  }
  
  // Fetches all keyrings for all given keyring IDs
  func keyrings(
    for keyringIds: [BraveWallet.KeyringId]
  ) async -> [BraveWallet.KeyringInfo] {
    var allKeyrings: [BraveWallet.KeyringInfo] = []
    allKeyrings = await withTaskGroup(
      of: BraveWallet.KeyringInfo.self,
      returning: [BraveWallet.KeyringInfo].self,
      body: { @MainActor group in
        for keyringId in keyringIds {
          group.addTask { @MainActor in
            await self.keyringInfo(keyringId)
          }
        }
        return await group.reduce([BraveWallet.KeyringInfo](), { partialResult, prior in
          return partialResult + [prior]
        })
        .sorted(by: { lhs, rhs in
          (lhs.coin ?? .eth).sortOrder < (rhs.coin ?? .eth).sortOrder
        })
      }
    )
    return allKeyrings
  }
  
  /// Check if any wallet account has been created given a coin type and a chain id
  @MainActor func isAccountAvailable(for coin: BraveWallet.CoinType, chainId: String) async -> Bool {
    let keyringId = BraveWallet.KeyringId.keyringId(for: coin, on: chainId)
    let keyringInfo = await keyringInfo(keyringId)
    // If user restore a wallet, `BraveWallet.KeyringInfo.isKeyringCreated` can be true,
    // but `BraveWallet.KeyringInfo.accountInfos` will be empty.
    // Hence, we will have to check if `BraveWallet.KeyringInfo.accountInfos` is empty instead of
    // checking the boolean of `BraveWallet.KeyringInfo.isKeyringCreated`
    return !keyringInfo.accountInfos.isEmpty
  }
}
