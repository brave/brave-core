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
  
  func testUpdate() {
    let formatter = WeiFormatter(decimalFormatStyle: .decimals(precision: 18))
    let currencyFormatter = NumberFormatter().then { $0.numberStyle = .currency }
    
    // config Solana
    let mockSolAccountInfos: [BraveWallet.AccountInfo] = [.mockSolAccount]
    let solNetwork: BraveWallet.NetworkInfo = .mockSolana
    let mockSolUserAssets: [BraveWallet.BlockchainToken] = [
      BraveWallet.NetworkInfo.mockSolana.nativeToken.then { $0.visible = true },
      .mockSpdToken.then { $0.visible = false }, // Verify non-visible assets not displayed #6386
      .mockSolanaNFTToken
    ]
    let mockLamportBalance: UInt64 = 3876535000 // ~3.8765 SOL
    let mockSolDecimalBalance: Double = 3.8765 // rounded
    let mockNFTBalance: Double = 1
    let mockSolAssetPrice: BraveWallet.AssetPrice = .init(fromAsset: "sol", toAsset: "usd", price: "200.00", assetTimeframeChange: "-57.23")
    let mockSolPriceHistory: [BraveWallet.AssetTimePrice] = [.init(date: Date(timeIntervalSinceNow: -1000), price: "$200.00"), .init(date: Date(), price: "250.00")]
    let totalSolBalanceValue: Double = (Double(mockSolAssetPrice.price) ?? 0) * mockSolDecimalBalance
    
    // config Ethereum
    let mockEthAccountInfos: [BraveWallet.AccountInfo] = [.mockEthAccount]
    let ethNetwork: BraveWallet.NetworkInfo = .mockMainnet
    let mockEthUserAssets: [BraveWallet.BlockchainToken] = [
      .previewToken.then { $0.visible = true },
      .mockUSDCToken.then { $0.visible = false }, // Verify non-visible assets not displayed #6386
      .mockERC721NFTToken
    ]
    let mockEthDecimalBalance: Double = 0.0896
    let numEthDecimals = Int(mockEthUserAssets[0].decimals)
    let mockBalanceWei = formatter.weiString(from: 0.0896, radix: .hex, decimals: numEthDecimals) ?? ""
    let mockNFTBalanceWei = formatter.weiString(from: 1, radix: .hex, decimals: 0) ?? ""
    let mockEthAssetPrice: BraveWallet.AssetPrice = .init(fromAsset: "eth", toAsset: "usd", price: "3059.99", assetTimeframeChange: "-57.23")
    let mockEthPriceHistory: [BraveWallet.AssetTimePrice] = [.init(date: Date(timeIntervalSinceNow: -1000), price: "$3000.00"), .init(date: Date(), price: "3059.99")]
    let totalEthBalanceValue: Double = (Double(mockEthAssetPrice.price) ?? 0) * mockEthDecimalBalance
    
    let totalBalanceValue = totalEthBalanceValue + totalSolBalanceValue
    let totalBalance = currencyFormatter.string(from: NSNumber(value: totalBalanceValue)) ?? ""
    
    // setup test services
    let keyringService = BraveWallet.TestKeyringService()
    keyringService._keyringInfo = { keyringId, completion in
      if keyringId == BraveWallet.DefaultKeyringId {
        let keyring: BraveWallet.KeyringInfo = .init(
          id: BraveWallet.DefaultKeyringId,
          isKeyringCreated: true,
          isLocked: false,
          isBackedUp: true,
          accountInfos: mockEthAccountInfos
        )
        completion(keyring)
      } else {
        let keyring: BraveWallet.KeyringInfo = .init(
          id: BraveWallet.SolanaKeyringId,
          isKeyringCreated: true,
          isLocked: false,
          isBackedUp: true,
          accountInfos: mockSolAccountInfos
        )
        completion(keyring)
      }
    }
    keyringService._addObserver = { _ in }
    keyringService._isLocked = { completion in
      // unlocked would cause `update()` from call in `init` to be called prior to test being setup.g
      completion(true)
    }
    let rpcService = BraveWallet.TestJsonRpcService()
    rpcService._addObserver = { _ in }
    rpcService._allNetworks = { coin, completion in
      switch coin {
      case .eth:
        completion([ethNetwork])
      case .sol:
        completion([solNetwork])
      case .fil:
        XCTFail("Should not fetch filecoin network")
      @unknown default:
        XCTFail("Should not fetch unknown network")
      }
    }
    rpcService._balance = { _, _, _, completion in
      completion(mockBalanceWei, .success, "") // eth balance
    }
    rpcService._erc721TokenBalance = { _, _, _, _, completion in
      completion(mockNFTBalanceWei, .success, "") // eth nft balance
    }
    rpcService._solanaBalance = { accountAddress, chainId, completion in
      completion(mockLamportBalance, .success, "") // sol balance
    }
    rpcService._splTokenAccountBalance = {_, _, _, completion in
      completion("\(mockNFTBalance)", UInt8(0), "\(mockNFTBalance)", .success, "") // sol nft balance
    }
    let walletService = BraveWallet.TestBraveWalletService()
    walletService._userAssets = { _, coin, completion in
      switch coin {
      case .eth:
        completion(mockEthUserAssets)
      case .sol:
        completion(mockSolUserAssets)
      case .fil:
        XCTFail("Should not fetch filecoin assets")
      @unknown default:
        XCTFail("Should not fetch unknown assets")
      }
    }
    walletService._addObserver = { _ in }
    walletService._defaultBaseCurrency = { $0(CurrencyCode.usd.code) }
    walletService._selectedCoin = { $0(BraveWallet.CoinType.eth) }
    let assetRatioService = BraveWallet.TestAssetRatioService()
    assetRatioService._price = { priceId, _, _, completion in
      completion(true, [mockEthAssetPrice, mockSolAssetPrice])
    }
    assetRatioService._priceHistory = { priceId, _, _, completion in
      if priceId == "sol" {
        completion(true, mockSolPriceHistory)
      } else {
        completion(true, mockEthPriceHistory)
      }
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
        XCTAssertEqual(lastUpdatedVisibleAssets.count, 2) // SOL on Solana mainnet, ETH on Ethereum mainnet
        XCTAssertEqual(lastUpdatedVisibleAssets[0].token.symbol, mockSolUserAssets.first?.symbol)
        XCTAssertEqual(lastUpdatedVisibleAssets[0].decimalBalance, mockSolDecimalBalance)
        XCTAssertEqual(lastUpdatedVisibleAssets[0].price, mockSolAssetPrice.price)
        XCTAssertEqual(lastUpdatedVisibleAssets[0].history, mockSolPriceHistory)
        
        XCTAssertEqual(lastUpdatedVisibleAssets[1].token.symbol, mockEthUserAssets.first?.symbol)
        XCTAssertEqual(lastUpdatedVisibleAssets[1].decimalBalance, mockEthDecimalBalance)
        XCTAssertEqual(lastUpdatedVisibleAssets[1].price, mockEthAssetPrice.price)
        XCTAssertEqual(lastUpdatedVisibleAssets[1].history, mockEthPriceHistory)
      }.store(in: &cancellables)
    // test that `update()` will assign new value to `userVisibleNFTs` publisher
    let userVisibleNFTsException = expectation(description: "update-userVisibleNFTs")
    XCTAssertTrue(store.userVisibleNFTs.isEmpty)  // Initial state
    store.$userVisibleNFTs
      .dropFirst()
      .collect(2)
      .sink { userVisibleNFTs in
        defer { userVisibleNFTsException.fulfill() }
        XCTAssertEqual(userVisibleNFTs.count, 2) // empty nfts, populated nfts
        guard let lastUpdatedVisibleNFTs = userVisibleNFTs.last else {
          XCTFail("Unexpected test result")
          return
        }
        XCTAssertEqual(lastUpdatedVisibleNFTs.count, 2)
        XCTAssertEqual(lastUpdatedVisibleNFTs[0].token.symbol, mockSolUserAssets.last?.symbol)
        XCTAssertEqual(lastUpdatedVisibleNFTs[0].balance, Int(mockNFTBalance))
        
        XCTAssertEqual(lastUpdatedVisibleNFTs[1].token.symbol, mockEthUserAssets.last?.symbol)
        XCTAssertEqual(lastUpdatedVisibleNFTs[1].balance, Int(mockNFTBalanceWei))
      }.store(in: &cancellables)
    // test that `update()` will assign new value to `balance` publisher
    let balanceException = expectation(description: "update-balance")
    store.$balance
      .dropFirst()
      .first()
      .sink { balance in
        defer { balanceException.fulfill() }
        XCTAssertEqual(balance, totalBalance)
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
