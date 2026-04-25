// Copyright 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveCore
import Foundation

extension BraveWalletAssetRatioService {
  /// Fetches the prices for a given list of `BlockchainToken`, giving a list of `AssetPrice`.
  /// Returns an empty array in case no prices are found.
  @MainActor func fetchPrices(
    for tokens: [BraveWallet.BlockchainToken],
    vsCurrency: String
  ) async -> [BraveWallet.AssetPrice] {
    let requests = tokens.map { token in
      BraveWallet.AssetPriceRequest(
        coin: token.coin,
        chainId: token.chainId,
        address: token.contractAddress.isEmpty ? nil : token.contractAddress
      )
    }

    let (_, prices) = await self.price(
      requests: requests,
      vsCurrency: vsCurrency
    )

    return prices
  }
}

extension BraveWallet.AssetPrice {
  /// Checks if this AssetPrice matches the given coin, chainId, and address
  /// Two tokens are considered the same if they have the same coin, chainId, and address
  func eq(coin: BraveWallet.CoinType, chainId: String, address: String) -> Bool {
    return self.coin == coin
      && self.chainId == chainId
      && self.address.lowercased() == address.lowercased()
  }
}

extension Array where Element == BraveWallet.AssetPrice {
  /// Get token price from a list of asset prices
  /// Returns the matching AssetPrice if found, nil otherwise
  func getTokenPrice(
    for token: BraveWallet.BlockchainToken
  ) -> BraveWallet.AssetPrice? {
    return first { assetPrice in
      assetPrice.eq(coin: token.coin, chainId: token.chainId, address: token.contractAddress)
    }
  }

  /// Updates the existing array with new AssetPrice objects, ensuring no duplicates
  /// Two AssetPrices are considered duplicates if they have the same coin, chainId, and address
  mutating func update(with newPrices: [BraveWallet.AssetPrice]) {
    for newPrice in newPrices {
      // Find index of existing price with same coin, chainId, and address
      if let existingIndex = firstIndex(where: { existingPrice in
        existingPrice.eq(coin: newPrice.coin, chainId: newPrice.chainId, address: newPrice.address)
      }) {
        // Replace existing price with new one
        self[existingIndex] = newPrice
      } else {
        // Add new price if no duplicate found
        append(newPrice)
      }
    }
  }
}
