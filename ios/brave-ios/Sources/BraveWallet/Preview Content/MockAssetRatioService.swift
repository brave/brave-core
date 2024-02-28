// Copyright 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveCore
import Foundation

#if DEBUG

class MockAssetRatioService: BraveWalletAssetRatioService {
  private let assets: [String: BraveWallet.AssetPrice] = [
    "eth": .init(
      fromAsset: "eth",
      toAsset: "usd",
      price: "3059.99",
      assetTimeframeChange: "-57.23"
    ),
    "bat": .init(
      fromAsset: "bat",
      toAsset: "usd",
      price: "0.627699",
      assetTimeframeChange: "-0.019865"
    ),
  ]
  func price(
    fromAssets: [String],
    toAssets: [String],
    timeframe: BraveWallet.AssetPriceTimeframe,
    completion: @escaping (Bool, [BraveWallet.AssetPrice]) -> Void
  ) {
    let prices = assets.filter { (key, value) in
      fromAssets.contains(where: { key == $0 })
    }
    completion(!prices.isEmpty, Array(prices.values))
  }

  func estimatedTime(_ gasPrice: String, completion: @escaping (Bool, String) -> Void) {
    completion(false, "")
  }

  func tokenInfo(
    _ contractAddress: String,
    completion: @escaping (BraveWallet.BlockchainToken?) -> Void
  ) {
    completion(nil)
  }

  func priceHistory(
    asset: String,
    vsAsset: String,
    timeframe: BraveWallet.AssetPriceTimeframe,
    completion: @escaping (Bool, [BraveWallet.AssetTimePrice]) -> Void
  ) {
    completion(false, [])
  }

  func buyUrlV1(
    provider: BraveWallet.OnRampProvider,
    chainId: String,
    address: String,
    symbol: String,
    amount: String,
    currencyCode: String,
    completion: @escaping (String, String?) -> Void
  ) {
    completion("", nil)
  }

  func sellUrl(
    provider: BraveWallet.OffRampProvider,
    chainId: String,
    symbol: String,
    amount: String,
    currencyCode: String,
    completion: @escaping (String, String?) -> Void
  ) {
    completion("", nil)
  }

  func coinMarkets(
    vsAsset: String,
    limit: UInt8,
    completion: @escaping (Bool, [BraveWallet.CoinMarket]) -> Void
  ) {
    completion(false, [])
  }
}

#endif
