// Copyright 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveCore
import Foundation

extension BraveWalletAssetRatioService {
  typealias PricesResult = (assetPrices: [BraveWallet.AssetPrice], failureCount: Int)

  /// Filter out empty strings, make unique, and ensure lowercase
  private func uniqueNonEmptyPriceIds(_ priceIds: [String]) -> [String] {
    Array(Set(priceIds.filter { !$0.isEmpty }.map { $0.lowercased() }))
  }

  /// Fetch the price for a list of assets to a list of assets over a given timeframe.
  /// If the main call for all asset prices fails, will fetch the price for each asset individually, so one failure will not result in a failure to fetch all assets.
  @MainActor func priceWithIndividualRetry(
    _ fromAssets: [String],
    toAssets: [String],
    timeframe: BraveWallet.AssetPriceTimeframe
  ) async -> PricesResult {
    let uniquePriceIds = uniqueNonEmptyPriceIds(fromAssets)

    let (success, prices) = await self.price(
      fromAssets: uniquePriceIds,
      toAssets: toAssets,
      timeframe: timeframe
    )
    guard success else {
      return await self.priceIndividually(
        uniquePriceIds,
        toAssets: toAssets,
        timeframe: timeframe
      )
    }
    return PricesResult(prices, 0)
  }

  /// Fetch the price for each asset in `fromAssets` individually, so one failure will not result in a failure to fetch all assets.
  @MainActor private func priceIndividually(
    _ fromAssets: [String],
    toAssets: [String],
    timeframe: BraveWallet.AssetPriceTimeframe
  ) async -> PricesResult {
    return await withTaskGroup(of: PricesResult.self) { @MainActor group -> PricesResult in
      fromAssets.forEach { asset in
        group.addTask { @MainActor in
          let (success, prices) = await self.price(
            fromAssets: [asset],
            toAssets: toAssets,
            timeframe: timeframe
          )
          return PricesResult(prices, success ? 0 : 1)
        }
      }
      return await group.reduce(
        PricesResult([], 0),
        { prior, new in
          return PricesResult(
            prior.assetPrices + new.assetPrices,
            prior.failureCount + new.failureCount
          )
        }
      )
    }
  }
}

extension BraveWalletAssetRatioService {
  /// Fetch the price for a list of assets to a list of assets over a given timeframe.
  /// If the main call for all asset prices fails, will fetch the price for each asset individually, so one failure will not result in a failure to fetch all assets.
  func priceWithIndividualRetry(
    _ fromAssets: [String],
    toAssets: [String],
    timeframe: BraveWallet.AssetPriceTimeframe,
    completion: @escaping (PricesResult) -> Void
  ) {
    let uniquePriceIds = uniqueNonEmptyPriceIds(fromAssets)

    price(fromAssets: uniquePriceIds, toAssets: toAssets, timeframe: timeframe) {
      [self] success, prices in
      guard success else {
        self.priceIndividually(
          uniquePriceIds,
          toAssets: toAssets,
          timeframe: timeframe,
          completion: completion
        )
        return
      }
      completion(PricesResult(prices, 0))
    }
  }

  /// Fetch the price for each asset in `fromAssets` individually, so one failure will not result in a failure to fetch all assets.
  private func priceIndividually(
    _ fromAssets: [String],
    toAssets: [String],
    timeframe: BraveWallet.AssetPriceTimeframe,
    completion: @escaping (PricesResult) -> Void
  ) {
    var pricesResults: [PricesResult] = []
    let dispatchGroup = DispatchGroup()
    fromAssets.forEach { asset in
      dispatchGroup.enter()
      self.price(fromAssets: [asset], toAssets: toAssets, timeframe: timeframe) { success, prices in
        defer { dispatchGroup.leave() }
        pricesResults.append(PricesResult(prices, success ? 0 : 1))
      }
    }

    dispatchGroup.notify(queue: .main) {
      let priceResult = pricesResults.reduce(
        PricesResult([], 0),
        { prior, new in
          return PricesResult(
            prior.assetPrices + new.assetPrices,
            prior.failureCount + new.failureCount
          )
        }
      )
      completion(priceResult)
    }
  }

  /// Fetches the prices for a given list of `assetRatioId`, giving a dictionary with the price for each symbol
  @MainActor func fetchPrices(
    for priceIds: [String],
    toAssets: [String],
    timeframe: BraveWallet.AssetPriceTimeframe
  ) async -> [String: String] {
    let uniquePriceIds = uniqueNonEmptyPriceIds(priceIds)

    let priceResult = await priceWithIndividualRetry(
      uniquePriceIds,
      toAssets: toAssets,
      timeframe: timeframe
    )
    let prices = Dictionary(
      uniqueKeysWithValues: priceResult.assetPrices.map { ($0.fromAsset, $0.price) }
    )
    return prices
  }
}
