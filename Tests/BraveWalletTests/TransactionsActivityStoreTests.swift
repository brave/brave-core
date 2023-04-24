// Copyright 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Combine
import XCTest
import BraveCore
@testable import BraveWallet

class TransactionsActivityStoreTests: XCTestCase {
  
  private var cancellables: Set<AnyCancellable> = .init()
  
  let networks: [BraveWallet.CoinType: [BraveWallet.NetworkInfo]] = [
    .eth: [.mockMainnet],
    .sol: [.mockSolana]
  ]
  let visibleAssetsForCoins: [BraveWallet.CoinType: [BraveWallet.BlockchainToken]] = [
    .eth: [
      BraveWallet.NetworkInfo.mockMainnet.nativeToken.copy(asVisibleAsset: true),
      .mockERC721NFTToken.copy(asVisibleAsset: true),
      .mockUSDCToken.copy(asVisibleAsset: true)],
    .sol: [
      BraveWallet.NetworkInfo.mockSolana.nativeToken.copy(asVisibleAsset: true),
      .mockSolanaNFTToken.copy(asVisibleAsset: true),
      .mockSpdToken.copy(asVisibleAsset: true)]
  ]
  let tokenRegistry: [BraveWallet.CoinType: [BraveWallet.BlockchainToken]] = [:]
  let mockAssetPrices: [BraveWallet.AssetPrice] = [
    .init(fromAsset: "eth", toAsset: "usd", price: "3059.99", assetTimeframeChange: "-57.23"),
    .init(fromAsset: "usdc", toAsset: "usd", price: "1.00", assetTimeframeChange: "-57.23"),
    .init(fromAsset: "sol", toAsset: "usd", price: "2.00", assetTimeframeChange: "-57.23"),
    .init(fromAsset: "spd", toAsset: "usd", price: "0.50", assetTimeframeChange: "-57.23")
  ]
  let transactions: [BraveWallet.CoinType: [BraveWallet.TransactionInfo]] = [
    .eth: [.previewConfirmedSend, .previewConfirmedSwap],
    .sol: [.previewConfirmedSolSystemTransfer]
  ]
  
  func testUpdate() {
    let keyringService = BraveWallet.TestKeyringService()
    keyringService._addObserver = { _ in }
    keyringService._keyringInfo = { keyringId, completion in
      if keyringId == BraveWallet.DefaultKeyringId {
        completion(.mockDefaultKeyringInfo)
      } else {
        completion(.mockSolanaKeyringInfo)
      }
    }
    
    let rpcService = BraveWallet.TestJsonRpcService()
    rpcService._network = { coin, completion in
      if coin == .sol {
        completion(.mockSolana)
      } else {
        completion(.mockMainnet)
      }
    }
    
    let walletService = BraveWallet.TestBraveWalletService()
    walletService._defaultBaseCurrency = { $0(CurrencyCode.usd.code) }
    walletService._userAssets = { chainId, coin, completion in
      completion(self.visibleAssetsForCoins[coin] ?? [])
    }
    
    let assetRatioService = BraveWallet.TestAssetRatioService()
    assetRatioService._price = { _, _, _, completion in
      completion(true, self.mockAssetPrices)
    }
    
    let blockchainRegistry = BraveWallet.TestBlockchainRegistry()
    blockchainRegistry._allTokens = { chainId, coin, completion in
      completion(self.tokenRegistry[coin] ?? [])
    }
    
    let txService = BraveWallet.TestTxService()
    txService._addObserver = { _ in }
    txService._allTransactionInfo = { coin, accountAddress, completion in
      completion(self.transactions[coin] ?? [])
    }
    
    let solTxManagerProxy = BraveWallet.TestSolanaTxManagerProxy()
    solTxManagerProxy._estimatedTxFee = { $1(UInt64(1), .success, "") }
    
    let store = TransactionsActivityStore(
      keyringService: keyringService,
      rpcService: rpcService,
      walletService: walletService,
      assetRatioService: assetRatioService,
      blockchainRegistry: blockchainRegistry,
      txService: txService,
      solTxManagerProxy: solTxManagerProxy
    )
    
    
    let transactionsExpectation = expectation(description: "transactionsExpectation")
    store.$transactionSummaries
      .dropFirst()
      .collect(2) // without asset prices, with asset prices
      .sink { transactionSummariesUpdates in
        defer { transactionsExpectation.fulfill() }
        guard let transactionSummariesWithoutPrices = transactionSummariesUpdates.first,
              let transactionSummariesWithPrices = transactionSummariesUpdates[safe: 1] else {
          XCTFail("Expected 2 updates to transactionSummaries")
          return
        }
        let expectedTransactions = self.transactions.values.flatMap { $0 }
        // verify all transactions from supported coin types are shown
        XCTAssertEqual(transactionSummariesWithoutPrices.count, expectedTransactions.count)
        XCTAssertEqual(transactionSummariesWithPrices.count, expectedTransactions.count)
        // verify sorted by `createdTime`
        let expectedSortedOrder = expectedTransactions.sorted(by: { $0.createdTime > $1.createdTime })
        XCTAssertEqual(transactionSummariesWithoutPrices.map(\.txInfo.txHash), expectedSortedOrder.map(\.txHash))
        XCTAssertEqual(transactionSummariesWithPrices.map(\.txInfo.txHash), expectedSortedOrder.map(\.txHash))
        // verify they are populated with correct tx (summaries are tested in `TransactionParserTests`)
        XCTAssertEqual(transactionSummariesWithoutPrices[safe: 0]?.txInfo, self.transactions[.sol]?[safe: 0] ?? .init())
        XCTAssertEqual(transactionSummariesWithPrices[safe: 0]?.txInfo, self.transactions[.sol]?[safe: 0] ?? .init())
        XCTAssertEqual(transactionSummariesWithoutPrices[safe: 1]?.txInfo, self.transactions[.eth]?[safe: 0] ?? .init())
        XCTAssertEqual(transactionSummariesWithPrices[safe: 0]?.txInfo, self.transactions[.sol]?[safe: 0] ?? .init())
        XCTAssertEqual(transactionSummariesWithoutPrices[safe: 2]?.txInfo, self.transactions[.eth]?[safe: 1] ?? .init())
        XCTAssertEqual(transactionSummariesWithPrices[safe: 2]?.txInfo, self.transactions[.eth]?[safe: 1] ?? .init())
        
        // verify gas fee fiat
        XCTAssertEqual(transactionSummariesWithPrices[safe: 0]?.gasFee?.fiat, "$0.000000002")
        XCTAssertEqual(transactionSummariesWithPrices[safe: 1]?.gasFee?.fiat, "$10.41008598")
        XCTAssertEqual(transactionSummariesWithPrices[safe: 2]?.gasFee?.fiat, "$255.03792654")
      }
      .store(in: &cancellables)
    
    store.update()
    
    waitForExpectations(timeout: 1) { error in
      XCTAssertNil(error)
    }
  }
}
