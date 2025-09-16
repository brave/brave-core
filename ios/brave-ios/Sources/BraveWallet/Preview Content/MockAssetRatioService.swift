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
      coin: .eth,
      chainId: BraveWallet.MainnetChainId,
      address: "",
      price: "3059.99",
      vsCurrency: "usd",
      cacheStatus: .hit,
      source: .coingecko,
      percentageChange24h: "-57.23"
    ),
    "bat": .init(
      coin: .eth,
      chainId: BraveWallet.MainnetChainId,
      address: "0x0d8775f648430679a709e98d2b0cb6250d2887ef",
      price: "0.627699",
      vsCurrency: "usd",
      cacheStatus: .hit,
      source: .coingecko,
      percentageChange24h: "-0.019865"
    ),
  ]

  func price(
    requests: [BraveWallet.AssetPriceRequest],
    vsCurrency: String,
    completion: @escaping (Bool, [BraveWallet.AssetPrice]) -> Void
  ) {
    let prices = requests.compactMap { request in
      // Find matching asset based on coin type, chain ID, and address
      return assets.first { (_, assetPrice) in
        assetPrice.coin == request.coin
          && assetPrice.chainId == request.chainId
          && assetPrice.address == request.address
      }?.value
    }
    completion(!prices.isEmpty, prices)
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
