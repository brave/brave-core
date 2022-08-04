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
        var allKeyrings: [BraveWallet.KeyringInfo] = []
        for await keyring in group {
          allKeyrings.append(keyring)
        }
        return allKeyrings
      }
    )
    return allKeyrings
  }
}
