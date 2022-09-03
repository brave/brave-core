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
        for coin in coins {
          group.addTask { @MainActor in
            await self.keyringInfo(coin.keyringId)
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
  
  // Fetches all default accounts for each of the given coin types
  func defaultAccounts(
    for coins: OrderedSet<BraveWallet.CoinType>
  ) async -> [BraveWallet.AccountInfo] {
    return await withTaskGroup(
      of: BraveWallet.AccountInfo?.self,
      body: { @MainActor group in
        for coin in coins {
          group.addTask { @MainActor in
            let accounts = await self.keyringInfo(coin.keyringId).accountInfos
            let selectedAccountAddress = await self.selectedAccount(coin)
            return accounts.first(where: { $0.address.caseInsensitiveCompare(selectedAccountAddress ?? "") == .orderedSame })
          }
        }
        var defaultAccounts: [BraveWallet.AccountInfo] = []
        for await account in group {
          if let account = account {
            defaultAccounts.append(account)
          }
        }
        return defaultAccounts
      })
  }
}
