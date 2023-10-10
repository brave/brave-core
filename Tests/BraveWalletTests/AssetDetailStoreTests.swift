// Copyright 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import Combine
import XCTest
import BraveCore
@testable import BraveWallet

class AssetDetailStoreTests: XCTestCase {
  private var cancellables: Set<AnyCancellable> = .init()
  
  func testUpdateWithBlockchainToken() {
    let currencyFormatter = NumberFormatter().then { $0.numberStyle = .currency }
    let formatter = WeiFormatter(decimalFormatStyle: .decimals(precision: 18))
    
    let assetRatioService = BraveWallet.TestAssetRatioService()
    assetRatioService._price = { _, _, _, completion in
      completion(true, [.init(fromAsset: BraveWallet.BlockchainToken.previewToken.tokenId, toAsset: "btc", price: "0.1", assetTimeframeChange: "1"), .init(fromAsset: BraveWallet.BlockchainToken.previewToken.tokenId, toAsset: "usd", price: "1", assetTimeframeChange: "1")])
    }
    assetRatioService._priceHistory = { _, _, _, completion in
      completion(true, [.init(date: Date(), price: "0.99")])
    }
    
    let keyringService = BraveWallet.TestKeyringService()
    keyringService._keyringInfo = {
      $1(.mockDefaultKeyringInfo)
    }
    keyringService._addObserver = { _ in }
    
    let mockEthBalance: Double = 1
    let ethBalanceWei = formatter.weiString(
      from: mockEthBalance,
      radix: .hex,
      decimals: Int(BraveWallet.BlockchainToken.previewToken.decimals)
    ) ?? ""
    let formattedEthBalance = currencyFormatter.string(from: NSNumber(value: mockEthBalance)) ?? ""
    let rpcService = BraveWallet.TestJsonRpcService()
    rpcService._allNetworks = {
      $1([.mockMainnet])
    }
    rpcService._network = {
      $2(.mockMainnet)
    }
    rpcService._balance = { _, _, _, completion in
      completion(ethBalanceWei, .success, "")
    }
    
    let walletService = BraveWallet.TestBraveWalletService()
    walletService._defaultBaseCurrency = {
      $0("usd")
    }
    walletService._addObserver = { _ in }
    
    let mockAssetManager = TestableWalletUserAssetManager()
    mockAssetManager._getAllUserAssetsInNetworkAssets = { _, _ in
      [NetworkAssets(network: .mockMainnet, tokens: [.previewToken], sortOrder: 0)]
    }
    
    let txService = BraveWallet.TestTxService()
    txService._allTransactionInfo = {
      $3([.previewConfirmedSend])
    }
    txService._addObserver = { _ in }
    
    let blockchainRegistry = BraveWallet.TestBlockchainRegistry()
    blockchainRegistry._buyTokens = {
      $2([.previewToken])
    }
    blockchainRegistry._allTokens = {
      $2([.previewToken])
    }
    
    let solTxManagerProxy = BraveWallet.TestSolanaTxManagerProxy()
    solTxManagerProxy._estimatedTxFee = {
      $2(UInt64(0.1), .success, "")
    }
    
    let swapService = BraveWallet.TestSwapService()
    swapService._isSwapSupported = {
      $1(true)
    }
    
    // setup store
    let store = AssetDetailStore(
      assetRatioService: assetRatioService,
      keyringService: keyringService,
      rpcService: rpcService,
      walletService: walletService,
      txService: txService,
      blockchainRegistry: blockchainRegistry,
      solTxManagerProxy: solTxManagerProxy,
      swapService: swapService,
      userAssetManager: mockAssetManager,
      assetDetailType: .blockchainToken(.previewToken)
    )
    
    let assetDetailException = expectation(description: "update-blockchainToken")
    assetDetailException.expectedFulfillmentCount = 14
    store.$network
      .dropFirst()
      .sink { network in
        defer { assetDetailException.fulfill() }
        XCTAssertEqual(network, .mockMainnet)
      }
      .store(in: &cancellables)
    store.$isBuySupported
      .dropFirst()
      .sink {
        defer { assetDetailException.fulfill() }
        XCTAssertTrue($0)
      }
      .store(in: &cancellables)
    store.$isSendSupported
      .dropFirst()
      .sink {
        defer { assetDetailException.fulfill() }
        XCTAssertTrue($0)
      }
      .store(in: &cancellables)
    store.$isSwapSupported
      .dropFirst()
      .sink {
        defer { assetDetailException.fulfill() }
        XCTAssertTrue($0)
      }
      .store(in: &cancellables)
    store.$btcRatio
      .dropFirst()
      .sink {
        defer { assetDetailException.fulfill() }
        XCTAssertEqual($0, "0.1 BTC")
      }
      .store(in: &cancellables)
    store.$priceHistory
      .dropFirst()
      .sink { priceHistory in
        defer { assetDetailException.fulfill() }
        XCTAssertEqual(priceHistory.count, 1)
        XCTAssertEqual(priceHistory[0].price, "0.99")
      }
      .store(in: &cancellables)
    store.$price
      .dropFirst()
      .sink {
        defer { assetDetailException.fulfill() }
        XCTAssertEqual($0, "$1.00")
      }
      .store(in: &cancellables)
    store.$priceIsDown
      .dropFirst()
      .sink {
        defer { assetDetailException.fulfill() }
        XCTAssertFalse($0)
      }
      .store(in: &cancellables)
    store.$priceDelta
      .dropFirst()
      .sink {
        defer { assetDetailException.fulfill() }
        XCTAssertEqual($0, "1%")
      }
      .store(in: &cancellables)
    store.$accounts
      .dropFirst()
      .sink { accounts in
        defer { assetDetailException.fulfill() }
        XCTAssertEqual(accounts.count, 1)
        XCTAssertEqual(accounts[0].account, .mockEthAccount)
        XCTAssertEqual(accounts[0].balance, String(format: "%.4f", mockEthBalance))
        XCTAssertEqual(accounts[0].fiatBalance, formattedEthBalance)
      }
      .store(in: &cancellables)
    store.$transactionSummaries
      .dropFirst()
      .sink { tx in
        defer { assetDetailException.fulfill() }
        XCTAssertEqual(tx.count, 1)
        XCTAssertEqual(tx[0].txInfo.id, BraveWallet.TransactionInfo.previewConfirmedSend.id)
      }
      .store(in: &cancellables)
    store.$isLoadingPrice
      .dropFirst()
      .collect(2)
      .sink { values in
        defer { assetDetailException.fulfill() }
        guard let value = values.last
        else {
          XCTFail("Unexpected isLoadingPrice")
          return
        }
        XCTAssertFalse(value)
      }
      .store(in: &cancellables)
    store.$isInitialState
      .dropFirst()
      .sink {
        defer { assetDetailException.fulfill() }
        XCTAssertFalse($0)
      }
      .store(in: &cancellables)
    store.$isLoadingChart
      .dropFirst()
      .collect(2)
      .sink { values in
        defer { assetDetailException.fulfill() }
        guard let value = values.last
        else {
          XCTFail("Unexpected isLoadingChart")
          return
        }
        XCTAssertFalse(value)
      }
      .store(in: &cancellables)
    
    store.update()
    wait(for: [assetDetailException], timeout: 1)
  }
  
  func testUpdateWithCoinMarket() {
    let currencyFormatter = NumberFormatter().then { $0.numberStyle = .currency }
    let formatter = WeiFormatter(decimalFormatStyle: .decimals(precision: 18))
    
    let assetRatioService = BraveWallet.TestAssetRatioService()
    assetRatioService._price = { _, _, _, completion in
      completion(true, [.init(fromAsset: BraveWallet.BlockchainToken.previewToken.tokenId, toAsset: "btc", price: "0.1", assetTimeframeChange: "1")])
    }
    assetRatioService._priceHistory = { _, _, _, completion in
      completion(true, [.init(date: Date(), price: "0.99")])
    }
    
    let keyring = BraveWallet.KeyringInfo.mockDefaultKeyringInfo
    let keyringService = BraveWallet.TestKeyringService()
    keyringService._keyringInfo = {
      $1(keyring)
    }
    keyringService._addObserver = { _ in }
    keyringService._allAccounts = { completion in
      completion(.init(
        accounts: keyring.accountInfos,
        selectedAccount: keyring.accountInfos.first,
        ethDappSelectedAccount: keyring.accountInfos.first,
        solDappSelectedAccount: nil
      ))
    }
    
    let mockEthBalance: Double = 1
    let ethBalanceWei = formatter.weiString(
      from: mockEthBalance,
      radix: .hex,
      decimals: Int(BraveWallet.BlockchainToken.previewToken.decimals)
    ) ?? ""
    let formattedEthBalance = currencyFormatter.string(from: NSNumber(value: mockEthBalance)) ?? ""
    let rpcService = BraveWallet.TestJsonRpcService()
    rpcService._network = {
      $2(.mockMainnet)
    }
    rpcService._balance = { _, _, _, completion in
      completion(ethBalanceWei, .success, "")
    }
    
    let walletService = BraveWallet.TestBraveWalletService()
    walletService._defaultBaseCurrency = {
      $0("usd")
    }
    walletService._addObserver = { _ in }
    
    let mockAssetManager = TestableWalletUserAssetManager()
    mockAssetManager._getAllUserAssetsInNetworkAssets = { _, _ in
      [NetworkAssets(network: .mockMainnet, tokens: [.previewToken], sortOrder: 0)]
    }
    
    let txService = BraveWallet.TestTxService()
    txService._addObserver = { _ in }
    
    let blockchainRegistry = BraveWallet.TestBlockchainRegistry()
    blockchainRegistry._buyTokens = {
      $2([.previewToken])
    }
    blockchainRegistry._allTokens = {
      $2([.previewToken])
    }
    
    let solTxManagerProxy = BraveWallet.TestSolanaTxManagerProxy()
    let swapService = BraveWallet.TestSwapService()
    
    // setup store
    var store = AssetDetailStore(
      assetRatioService: assetRatioService,
      keyringService: keyringService,
      rpcService: rpcService,
      walletService: walletService,
      txService: txService,
      blockchainRegistry: blockchainRegistry,
      solTxManagerProxy: solTxManagerProxy,
      swapService: swapService,
      userAssetManager: mockAssetManager,
      assetDetailType: .coinMarket(.mockCoinMarketBitcoin)
    )
    
    let assetDetailBitcoinException = expectation(description: "update-coinMarket-bitcoin")
    assetDetailBitcoinException.expectedFulfillmentCount = 11
    store.$isBuySupported
      .dropFirst()
      .sink {
        defer { assetDetailBitcoinException.fulfill() }
        XCTAssertFalse($0)
      }
      .store(in: &cancellables)
    store.$isSendSupported
      .dropFirst()
      .sink {
        defer { assetDetailBitcoinException.fulfill() }
        XCTAssertFalse($0)
      }
      .store(in: &cancellables)
    store.$isSwapSupported
      .dropFirst()
      .sink {
        defer { assetDetailBitcoinException.fulfill() }
        XCTAssertFalse($0)
      }
      .store(in: &cancellables)
    store.$btcRatio
      .dropFirst()
      .sink {
        defer { assetDetailBitcoinException.fulfill() }
        XCTAssertEqual($0, "1 BTC")
      }
      .store(in: &cancellables)
    store.$priceHistory
      .dropFirst()
      .sink { priceHistory in
        defer { assetDetailBitcoinException.fulfill() }
        XCTAssertEqual(priceHistory.count, 1)
        XCTAssertEqual(priceHistory[0].price, "0.99")
      }
      .store(in: &cancellables)
    store.$price
      .dropFirst()
      .sink {
        defer { assetDetailBitcoinException.fulfill() }
        XCTAssertEqual($0, "$28,324.00")
      }
      .store(in: &cancellables)
    store.$priceIsDown
      .dropFirst()
      .sink {
        defer { assetDetailBitcoinException.fulfill() }
        XCTAssertFalse($0)
      }
      .store(in: &cancellables)
    store.$priceDelta
      .dropFirst()
      .sink {
        defer { assetDetailBitcoinException.fulfill() }
        XCTAssertEqual($0, "0.58%")
      }
      .store(in: &cancellables)
    store.$isLoadingPrice
      .dropFirst()
      .collect(2)
      .sink { values in
        defer { assetDetailBitcoinException.fulfill() }
        guard let value = values.last
        else {
          XCTFail("Unexpected isLoadingPrice")
          return
        }
        XCTAssertFalse(value)
      }
      .store(in: &cancellables)
    store.$isInitialState
      .dropFirst()
      .sink {
        defer { assetDetailBitcoinException.fulfill() }
        XCTAssertFalse($0)
      }
      .store(in: &cancellables)
    store.$isLoadingChart
      .dropFirst()
      .collect(2)
      .sink { values in
        defer {
          XCTAssertNil(store.network)
          XCTAssertTrue(store.accounts.isEmpty)
          XCTAssertTrue(store.transactionSummaries.isEmpty)

          assetDetailBitcoinException.fulfill()
        }
        guard let value = values.last
        else {
          XCTFail("Unexpected isLoadingChart")
          return
        }
        XCTAssertFalse(value)
      }
      .store(in: &cancellables)
    
    store.update()
    wait(for: [assetDetailBitcoinException], timeout: 1)
    cancellables.removeAll()
    
    // setup store
    store = AssetDetailStore(
      assetRatioService: assetRatioService,
      keyringService: keyringService,
      rpcService: rpcService,
      walletService: walletService,
      txService: txService,
      blockchainRegistry: blockchainRegistry,
      solTxManagerProxy: solTxManagerProxy,
      swapService: swapService,
      userAssetManager: mockAssetManager,
      assetDetailType: .coinMarket(.mockCoinMarketEth)
    )
    let assetDetailNonBitcoinException = expectation(description: "update-coinMarket-non-bitcoin")
    assetDetailNonBitcoinException.expectedFulfillmentCount = 11
    store.$isBuySupported
      .dropFirst()
      .sink {
        defer { assetDetailNonBitcoinException.fulfill() }
        XCTAssertTrue($0)
      }
      .store(in: &cancellables)
    store.$isSendSupported
      .dropFirst()
      .sink {
        defer { assetDetailNonBitcoinException.fulfill() }
        XCTAssertFalse($0)
      }
      .store(in: &cancellables)
    store.$isSwapSupported
      .dropFirst()
      .sink {
        defer { assetDetailNonBitcoinException.fulfill() }
        XCTAssertFalse($0)
      }
      .store(in: &cancellables)
    store.$btcRatio
      .dropFirst()
      .sink {
        defer { assetDetailNonBitcoinException.fulfill() }
        XCTAssertEqual($0, "0.1 BTC")
      }
      .store(in: &cancellables)
    store.$priceHistory
      .dropFirst()
      .sink { priceHistory in
        defer { assetDetailNonBitcoinException.fulfill() }
        XCTAssertEqual(priceHistory.count, 1)
        XCTAssertEqual(priceHistory[0].price, "0.99")
      }
      .store(in: &cancellables)
    store.$price
      .dropFirst()
      .sink {
        defer { assetDetailNonBitcoinException.fulfill() }
        XCTAssertEqual($0, "$1,860.57")
      }
      .store(in: &cancellables)
    store.$priceIsDown
      .dropFirst()
      .sink {
        defer { assetDetailNonBitcoinException.fulfill() }
        XCTAssertTrue($0)
      }
      .store(in: &cancellables)
    store.$priceDelta
      .dropFirst()
      .sink {
        defer {
          XCTAssertNil(store.network)
          XCTAssertTrue(store.accounts.isEmpty)
          XCTAssertTrue(store.transactionSummaries.isEmpty)
          
          assetDetailNonBitcoinException.fulfill()
        }
        XCTAssertEqual($0, "-0.23%")
      }
      .store(in: &cancellables)
    store.$isLoadingPrice
      .dropFirst()
      .collect(2)
      .sink { values in
        defer { assetDetailNonBitcoinException.fulfill() }
        guard let value = values.last
        else {
          XCTFail("Unexpected isLoadingPrice")
          return
        }
        XCTAssertFalse(value)
      }
      .store(in: &cancellables)
    store.$isInitialState
      .dropFirst()
      .sink {
        defer { assetDetailNonBitcoinException.fulfill() }
        XCTAssertFalse($0)
      }
      .store(in: &cancellables)
    store.$isLoadingChart
      .dropFirst()
      .collect(2)
      .sink { values in
        defer {
          XCTAssertNil(store.network)
          XCTAssertTrue(store.accounts.isEmpty)
          XCTAssertTrue(store.transactionSummaries.isEmpty)
          
          assetDetailNonBitcoinException.fulfill()
        }
        guard let value = values.last
        else {
          XCTFail("Unexpected isLoadingChart")
          return
        }
        XCTAssertFalse(value)
      }
      .store(in: &cancellables)
    
    store.update()
    wait(for: [assetDetailNonBitcoinException], timeout: 1)
  }
}
