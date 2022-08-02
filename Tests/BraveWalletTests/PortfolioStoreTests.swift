// Copyright 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Combine
import XCTest
import BraveCore
@testable import BraveWallet

class PortfolioStoreTests: XCTestCase {

  private var cancellables: Set<AnyCancellable> = .init()

  func testUpdateEthereum() {
    // config test
    let mockAccountInfos: [BraveWallet.AccountInfo] = [.mockEthAccount]
    let chainId = BraveWallet.MainnetChainId
    let network: BraveWallet.NetworkInfo = .mockMainnet
    let mockUserAssets: [BraveWallet.BlockchainToken] = [.previewToken.then { $0.visible = true }]
    let mockDecimalBalance: Double = 0.0896
    let numDecimals = Int(mockUserAssets[0].decimals)
    let formatter = WeiFormatter(decimalFormatStyle: .decimals(precision: numDecimals))
    let mockBalanceWei = formatter.weiString(from: 0.0896, radix: .hex, decimals: numDecimals) ?? ""
    let mockEthAssetPrice: BraveWallet.AssetPrice = .init(fromAsset: "eth", toAsset: "usd", price: "3059.99", assetTimeframeChange: "-57.23")
    let mockEthPriceHistory: [BraveWallet.AssetTimePrice] = [.init(date: Date(timeIntervalSinceNow: -1000), price: "$3000.00"), .init(date: Date(), price: "3059.99")]
    let totalEthBalanceValue: Double = (Double(mockEthAssetPrice.price) ?? 0) * mockDecimalBalance
    let currencyFormatter = NumberFormatter().then { $0.numberStyle = .currency }
    let totalEthBalance = currencyFormatter.string(from: NSNumber(value: totalEthBalanceValue)) ?? ""

    // setup test services
    let keyringService = BraveWallet.TestKeyringService()
    keyringService._keyringInfo = { _, completion in
      let keyring: BraveWallet.KeyringInfo = .init(
        id: BraveWallet.DefaultKeyringId,
        isKeyringCreated: true,
        isLocked: false,
        isBackedUp: true,
        accountInfos: mockAccountInfos)
      completion(keyring)
    }
    keyringService._addObserver = { _ in }
    keyringService._isLocked = { completion in
      // unlocked would cause `update()` from call in `init` to be called prior to test being setup.g
      completion(true)
    }
    let rpcService = BraveWallet.TestJsonRpcService()
    rpcService._addObserver = { _ in }
    rpcService._chainId = { $1(chainId) }
    rpcService._network = { $1(network) }
    rpcService._balance = { _, _, _, completion in
      completion(mockBalanceWei, .success, "")
    }
    let walletService = BraveWallet.TestBraveWalletService()
    walletService._userAssets = { _, _, completion in
      completion(mockUserAssets)
    }
    walletService._addObserver = { _ in }
    walletService._defaultBaseCurrency = { $0(CurrencyCode.usd.code) }
    walletService._selectedCoin = { $0(BraveWallet.CoinType.eth) }
    let assetRatioService = BraveWallet.TestAssetRatioService()
    assetRatioService._price = { _, _, _, completion in
      completion(true, [mockEthAssetPrice])
    }
    assetRatioService._priceHistory = { _, _, _, completion in
      completion(true, mockEthPriceHistory)
    }
    // setup store
    let store = PortfolioStore(
      keyringService: keyringService,
      rpcService: rpcService,
      walletService: walletService,
      assetRatioService: assetRatioService,
      blockchainRegistry: BraveWallet.TestBlockchainRegistry()
    )
    // test that `update()` will assign new value to `userVisibleAssets` publisher
    let userVisibleAssetsException = expectation(description: "update-userVisibleAssets")
    XCTAssertTrue(store.userVisibleAssets.isEmpty)  // Initial state
    store.$userVisibleAssets
      .dropFirst()
      .collect(2)
      .sink { userVisibleAssets in
        defer { userVisibleAssetsException.fulfill() }
        XCTAssertEqual(userVisibleAssets.count, 2) // empty assets, populated assets
        guard let lastUpdatedVisibleAssets = userVisibleAssets.last else {
          XCTFail("Unexpected test result")
          return
        }
        XCTAssertEqual(lastUpdatedVisibleAssets.count, 1)
        XCTAssertEqual(lastUpdatedVisibleAssets[0].decimalBalance, mockDecimalBalance)
        XCTAssertEqual(lastUpdatedVisibleAssets[0].price, mockEthAssetPrice.price)
        XCTAssertEqual(lastUpdatedVisibleAssets[0].history, mockEthPriceHistory)
      }.store(in: &cancellables)
    // test that `update()` will assign new value to `balance` publisher
    let balanceException = expectation(description: "update-balance")
    store.$balance
      .dropFirst()
      .first()
      .sink { balance in
        defer { balanceException.fulfill() }
        XCTAssertEqual(balance, totalEthBalance)
      }
      .store(in: &cancellables)
    // test that `update()` will update `isLoadingBalances` publisher
    let isLoadingBalancesException = expectation(description: "update-isLoadingBalances")
    store.$isLoadingBalances
      .dropFirst()
      .collect(2)
      .first()
      .sink { isLoadingUpdates in
        defer { isLoadingBalancesException.fulfill() }
        XCTAssertTrue(isLoadingUpdates[0])
        XCTAssertFalse(isLoadingUpdates[1])
      }
      .store(in: &cancellables)
    store.update()
    waitForExpectations(timeout: 1) { error in
      XCTAssertNil(error)
    }
  }
  
  func testUpdateSolana() {
    WalletDebugFlags.isSolanaEnabled = true
    // config test
    let mockAccountInfos: [BraveWallet.AccountInfo] = [.mockSolAccount]
    let network: BraveWallet.NetworkInfo = .mockSolana
    let chainId = network.chainId
    let mockUserAssets: [BraveWallet.BlockchainToken] = [BraveWallet.NetworkInfo.mockSolana.nativeToken.then { $0.visible = true }]
    let mockLamportBalance: UInt64 = 3876535000 // ~3.8765 SOL
    let mockDecimalBalance: Double = 3.8765 // rounded
    let mockSolAssetPrice: BraveWallet.AssetPrice = .init(fromAsset: "sol", toAsset: "usd", price: "200.00", assetTimeframeChange: "-57.23")
    let mockSolPriceHistory: [BraveWallet.AssetTimePrice] = [.init(date: Date(timeIntervalSinceNow: -1000), price: "$200.00"), .init(date: Date(), price: "250.00")]
    let totalSolBalanceValue: Double = (Double(mockSolAssetPrice.price) ?? 0) * mockDecimalBalance
    let currencyFormatter = NumberFormatter().then { $0.numberStyle = .currency }
    let totalSolBalance = currencyFormatter.string(from: NSNumber(value: totalSolBalanceValue)) ?? ""

    // setup test services
    let keyringService = BraveWallet.TestKeyringService()
    keyringService._keyringInfo = { _, completion in
      let keyring: BraveWallet.KeyringInfo = .init(
        id: BraveWallet.DefaultKeyringId,
        isKeyringCreated: true,
        isLocked: false,
        isBackedUp: true,
        accountInfos: mockAccountInfos)
      completion(keyring)
    }
    keyringService._addObserver = { _ in }
    keyringService._isLocked = { completion in
      // unlocked would cause `update()` from call in `init` to be called prior to test being setup.
      completion(true)
    }
    let rpcService = BraveWallet.TestJsonRpcService()
    rpcService._addObserver = { _ in }
    rpcService._chainId = { $1(chainId) }
    rpcService._network = { $1(network) }
    rpcService._solanaBalance = { accountAddress, chainId, completion in
      completion(mockLamportBalance, .success, "")
    }
    let walletService = BraveWallet.TestBraveWalletService()
    walletService._userAssets = { _, _, completion in
      completion(mockUserAssets)
    }
    walletService._addObserver = { _ in }
    walletService._defaultBaseCurrency = { $0(CurrencyCode.usd.code) }
    walletService._selectedCoin = { $0(BraveWallet.CoinType.sol) }
    let assetRatioService = BraveWallet.TestAssetRatioService()
    assetRatioService._price = { _, _, _, completion in
      completion(true, [mockSolAssetPrice])
    }
    assetRatioService._priceHistory = { _, _, _, completion in
      completion(true, mockSolPriceHistory)
    }
    // setup store
    let store = PortfolioStore(
      keyringService: keyringService,
      rpcService: rpcService,
      walletService: walletService,
      assetRatioService: assetRatioService,
      blockchainRegistry: BraveWallet.TestBlockchainRegistry()
    )
    // test that `update()` will assign new value to `userVisibleAssets` publisher
    let userVisibleAssetsException = expectation(description: "update-userVisibleAssets")
    XCTAssertTrue(store.userVisibleAssets.isEmpty)  // Initial state
    store.$userVisibleAssets
      .dropFirst()
      .collect(2)
      .sink { userVisibleAssets in
        defer { userVisibleAssetsException.fulfill() }
        XCTAssertEqual(userVisibleAssets.count, 2) // empty assets, populated assets
        guard let lastUpdatedVisibleAssets = userVisibleAssets.last else {
          XCTFail("Unexpected test result")
          return
        }
        XCTAssertEqual(lastUpdatedVisibleAssets.count, 1)
        XCTAssertEqual(lastUpdatedVisibleAssets[0].decimalBalance, mockDecimalBalance)
        XCTAssertEqual(lastUpdatedVisibleAssets[0].price, mockSolAssetPrice.price)
        XCTAssertEqual(lastUpdatedVisibleAssets[0].history, mockSolPriceHistory)
      }.store(in: &cancellables)
    // test that `update()` will assign new value to `balance` publisher
    let balanceException = expectation(description: "update-balance")
    store.$balance
      .dropFirst()
      .first()
      .sink { balance in
        defer { balanceException.fulfill() }
        XCTAssertEqual(balance, totalSolBalance)
      }
      .store(in: &cancellables)
    // test that `update()` will update `isLoadingBalances` publisher
    let isLoadingBalancesException = expectation(description: "update-isLoadingBalances")
    store.$isLoadingBalances
      .dropFirst()
      .collect(2)
      .first()
      .sink { isLoadingUpdates in
        defer { isLoadingBalancesException.fulfill() }
        XCTAssertTrue(isLoadingUpdates[0])
        XCTAssertFalse(isLoadingUpdates[1])
      }
      .store(in: &cancellables)
    store.update()
    waitForExpectations(timeout: 1) { error in
      XCTAssertNil(error)
    }
  }
}
