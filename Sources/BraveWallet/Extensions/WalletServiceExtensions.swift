// Copyright 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation
import BraveCore

public struct NetworkAssets: Equatable {
  let network: BraveWallet.NetworkInfo
  let tokens: [BraveWallet.BlockchainToken]
  let sortOrder: Int
}

extension BraveWalletBraveWalletService {
  /// - Warning: This  method is using `BraveCore` api to fetch user's assets stored in `BraveCore`
  /// which iOS has migrated and using `CoreData` instead since v1.53.x
  /// This should only be used for migration!
  /// Returns all the user assets for each of the given networks
  @MainActor func allUserAssets(in networks: [BraveWallet.NetworkInfo]) async -> [NetworkAssets] {
    await withTaskGroup(of: [NetworkAssets].self, body: { @MainActor group in
      for (index, network) in networks.enumerated() {
        group.addTask { @MainActor in
          let assets = await self.userAssets(network.chainId, coin: network.coin)
          return [NetworkAssets(network: network, tokens: assets, sortOrder: index)]
        }
      }
      return await group.reduce([NetworkAssets](), { $0 + $1 })
        .sorted(by: { $0.sortOrder < $1.sortOrder })
    })
  }
}
