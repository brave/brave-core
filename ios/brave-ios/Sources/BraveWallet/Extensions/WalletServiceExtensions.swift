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
  
  @MainActor func simpleHashSpamNFTs(for selectedAccounts: [BraveWallet.AccountInfo], on selectedNetworks: [BraveWallet.NetworkInfo]) async -> [NetworkAssets] {
    await withTaskGroup(of: [BraveWallet.BlockchainToken].self, body: { @MainActor group in
      for account in selectedAccounts {
        let networks = selectedNetworks
          .filter { $0.coin == account.coin }
          .map(\.chainId)
        group.addTask { @MainActor in
          let (spamNFTs, _) = await self.simpleHashSpamNfTs(account.address, chainIds: networks, coin: account.coin, cursor: nil)
          return spamNFTs
        }
      }
      var networkSpamNFTs: [NetworkAssets] = []
      let allSpamNFTs = await group.reduce([BraveWallet.BlockchainToken](), { $0 + $1 })
      for (index, network) in selectedNetworks.enumerated() {
        let spamNFTsOnNetwork = allSpamNFTs.filter { $0.chainId == network.chainId }
        networkSpamNFTs.append(NetworkAssets(network: network, tokens: spamNFTsOnNetwork, sortOrder: index))
      }
      return networkSpamNFTs
    })
  }
}
