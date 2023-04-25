// Copyright 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Combine
import XCTest
import BraveCore
@testable import BraveWallet

class MarketStoreTests: XCTestCase {
  private var cancellables: Set<AnyCancellable> = .init()
  
  private let mockCoinMarketList: [BraveWallet.CoinMarket] = [
    .init(
      id: "bitcoin",
      symbol: "btc",
      name: "Bitcoin",
      image: "https://assets.cgproxy.brave.com/coins/images/1/large/bitcoin.png?1547033579",
      marketCap: 542355803461,
      marketCapRank: 1,
      currentPrice: 28116,
      priceChange24h: -3.4252090135669278,
      priceChangePercentage24h: -0.01218,
      totalVolume: 16248623079
    ),
    .init(
      id: "ethereum",
      symbol: "eth",
      name: "Ethereum",
      image: "https://assets.cgproxy.brave.com/coins/images/279/large/ethereum.png?1595348880",
      marketCap: 228759772681,
      marketCapRank: 2,
      currentPrice: 1904.1199999999999,
      priceChange24h: 38.170000000000002,
      priceChangePercentage24h: 2.0457100000000001,
      totalVolume: 11515522814
    ),
    .init(
      id: "tether",
      symbol: "usdt",
      name: "Tether",
      image: "https://assets.cgproxy.brave.com/coins/images/325/large/Tether.png?1668148663",
      marketCap: 80107099039,
      marketCapRank: 3,
      currentPrice: 1,
      priceChange24h: 0.000071840000000000003,
      priceChangePercentage24h: 0.0071799999999999998,
      totalVolume: 27322892310
    ),
    .init(
      id: "binancecoin",
      symbol: "bnb",
      name: "BNB",
      image: "https://assets.cgproxy.brave.com/coins/images/825/large/bnb-icon2_2x.png?1644979850",
      marketCap: 49319769922,
      marketCapRank: 4,
      currentPrice: 312.47000000000003,
      priceChange24h: 1.48,
      priceChangePercentage24h: 0.47747000000000001,
      totalVolume: 670648968
    ),
    .init(
      id: "usd-coin",
      symbol: "usdc",
      name: "USD Coin",
      image: "https://assets.cgproxy.brave.com/coins/images/6319/large/USD_Coin_icon.png?1547042389",
      marketCap: 32636381218,
      marketCapRank: 5,
      currentPrice: 1.0009999999999999,
      priceChange24h: 0.0010566799999999999,
      priceChangePercentage24h: 0.10569000000000001,
      totalVolume: 4027572724
    ),
  ]
  
  func testUpdate() {
    let assetRatioService = BraveWallet.TestAssetRatioService()
    assetRatioService._coinMarkets = {_, _, completion in
      completion(true, self.mockCoinMarketList)
    }
    let blockchainRegistry = BraveWallet.TestBlockchainRegistry()
    blockchainRegistry._allTokens = {_, _, completion in
      completion([])
    }
    let rpcService = BraveWallet.TestJsonRpcService()
    rpcService._allNetworks = {_, completion in
      completion([.mockMainnet])
    }
    let walletService = BraveWallet.TestBraveWalletService()
    walletService._userAssets = {_, _, completion in
      completion([.previewToken])
    }
    walletService._defaultBaseCurrency = { $0(CurrencyCode.usd.code) }
    
    let store = MarketStore(
      assetRatioService: assetRatioService,
      blockchainRegistry: blockchainRegistry,
      rpcService: rpcService,
      walletService: walletService
    )
    let coinMarketExpectation = expectation(description: "coinMarketExpectation")
    store.$coins
      .dropFirst()
      .sink { coins in
        defer { coinMarketExpectation.fulfill() }
        XCTAssertTrue(coins.count == 5)
        XCTAssertEqual(coins[0], self.mockCoinMarketList[0])
        XCTAssertEqual(coins[1], self.mockCoinMarketList[1])
        XCTAssertEqual(coins[2], self.mockCoinMarketList[2])
        XCTAssertEqual(coins[3], self.mockCoinMarketList[3])
        XCTAssertEqual(coins[4], self.mockCoinMarketList[4])
      }
      .store(in: &cancellables)
    
    store.update()
    
    waitForExpectations(timeout: 1) { error in
      XCTAssertNil(error)
    }
  }
}
