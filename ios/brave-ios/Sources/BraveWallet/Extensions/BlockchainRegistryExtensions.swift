// Copyright 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation
import BraveCore

extension BraveWalletBlockchainRegistry {
  
  /// Returns all the `BlockchainToken`s for each of the given networks
  @MainActor func allTokens(in networks: [BraveWallet.NetworkInfo]) async -> [NetworkAssets] {
    await withTaskGroup(
      of: [NetworkAssets].self,
      body: { @MainActor group -> [NetworkAssets] in
        for (index, network) in networks.enumerated() {
          group.addTask { @MainActor in
            let allTokens = await self.allTokens(network.chainId, coin: network.coin)
            return [NetworkAssets(network: network, tokens: allTokens + [network.nativeToken], sortOrder: index)]
          }
        }
        return await group.reduce([NetworkAssets](), { $0 + $1 })
          .sorted(by: { $0.sortOrder < $1.sortOrder }) // maintain sort order of networks
      }
    )
  }
  
  /// Returns all buy-supported`BlockchainToken`s for each of the given network and a list
  /// on-ramp providers
  @MainActor func allBuyTokens(in network: BraveWallet.NetworkInfo, for providers: [BraveWallet.OnRampProvider]) async -> [BraveWallet.OnRampProvider: [BraveWallet.BlockchainToken]] {
    await withTaskGroup(
      of: [BraveWallet.OnRampProvider: [BraveWallet.BlockchainToken]].self,
      body: { @MainActor group -> [BraveWallet.OnRampProvider: [BraveWallet.BlockchainToken]] in
        for provider in providers {
          group.addTask { @MainActor in
            let allTokens = await self.buyTokens(provider, chainId: network.chainId)
            return [provider: allTokens]
          }
        }
        return await group.reduce([:], { $0.merging($1) { (_, new) in new } })
      }
    )
  }
}
