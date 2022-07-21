// Copyright 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation
import BraveCore

extension BraveWalletAssetRatioService {
  typealias PricesResult = (assetPrices: [BraveWallet.AssetPrice], failureCount: Int)
  
  /// Fetch the price for a list of assets to a list of assets over a given timeframe.
  /// If the main call for all asset prices fails, will fetch the price for each asset individually, so one failure will not result in a failure to fetch all assets.
  func priceWithIndividualRetry(
    _ fromAssets: [String],
    toAssets: [String],
    timeframe: BraveWallet.AssetPriceTimeframe
  ) async -> PricesResult {
    let (success, prices) = await self.price(fromAssets, toAssets: toAssets, timeframe: timeframe)
    guard success else {
      return await self.priceIndividually(fromAssets, toAssets: toAssets, timeframe: timeframe)
    }
    return PricesResult(prices, 0)
  }
  
  /// Fetch the price for each asset in `fromAssets` individually, so one failure will not result in a failure to fetch all assets.
  private func priceIndividually(
    _ fromAssets: [String],
    toAssets: [String],
    timeframe: BraveWallet.AssetPriceTimeframe
  ) async -> PricesResult {
    await withTaskGroup(of: PricesResult.self) { group -> PricesResult in
      fromAssets.forEach { asset in
        group.addTask {
          let (success, prices) = await self.price([asset], toAssets: toAssets, timeframe: timeframe)
          return PricesResult(prices, success ? 0 : 1)
        }
      }
      return await group.reduce(PricesResult([], 0), { prior, new in
        return PricesResult(prior.assetPrices + new.assetPrices, prior.failureCount + new.failureCount)
      })
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
    price(fromAssets, toAssets: toAssets, timeframe: timeframe) { [self] success, prices in
      guard success else {
        self.priceIndividually(fromAssets, toAssets: toAssets, timeframe: timeframe, completion: completion)
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
      self.price([asset], toAssets: toAssets, timeframe: timeframe) { success, prices in
        defer { dispatchGroup.leave() }
        pricesResults.append(PricesResult(prices, success ? 0 : 1))
      }
    }
    
    dispatchGroup.notify(queue: .main) {
      let priceResult = pricesResults.reduce(PricesResult([], 0), { prior, new in
        return PricesResult(prior.assetPrices + new.assetPrices, prior.failureCount + new.failureCount)
      })
      completion(priceResult)
    }
  }
}
