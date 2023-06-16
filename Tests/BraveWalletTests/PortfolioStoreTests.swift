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
  
  /// Test `update()` will fetch all visible user assets from all networks and display them sorted by their balance.
  func testUpdate() {
    let mockETHBalance: Double = 0.896
    let mockETHPrice: String = "3059.99" // ETH value = $2741.75104
    let mockUSDCBalance: Double = 4
    let mockUSDCPrice: String = "1" // USDC value = $4
    let mockSOLLamportBalance: UInt64 = 3876535000 // ~3.8765 SOL
    let mockSOLBalance: Double = 3.8765 // lamports rounded
    let mockSOLPrice: String = "200" // SOL value = $775.30
    
    let formatter = WeiFormatter(decimalFormatStyle: .decimals(precision: 18))
    let currencyFormatter = NumberFormatter().then { $0.numberStyle = .currency }
    
    // config Solana
    let mockSolAccountInfos: [BraveWallet.AccountInfo] = [.mockSolAccount]
    let solNetwork: BraveWallet.NetworkInfo = .mockSolana
    let mockSolUserAssets: [BraveWallet.BlockchainToken] = [
      BraveWallet.NetworkInfo.mockSolana.nativeToken.copy(asVisibleAsset: true),
      .mockSpdToken, // Verify non-visible assets not displayed #6386
      .mockSolanaNFTToken
    ]
    let mockNFTBalance: Double = 1
    let mockSOLAssetPrice: BraveWallet.AssetPrice = .init(fromAsset: "sol", toAsset: "usd", price: mockSOLPrice, assetTimeframeChange: "-57.23")
    let mockSOLPriceHistory: [BraveWallet.AssetTimePrice] = [
      .init(date: Date(timeIntervalSinceNow: -1000), price: mockSOLPrice),
      .init(date: Date(), price: "250.00")
    ]
    let totalSolBalanceValue: Double = (Double(mockSOLAssetPrice.price) ?? 0) * mockSOLBalance
    
    // config Ethereum
    let mockEthAccountInfos: [BraveWallet.AccountInfo] = [.mockEthAccount]
    let ethNetwork: BraveWallet.NetworkInfo = .mockMainnet
    let mockEthUserAssets: [BraveWallet.BlockchainToken] = [
      .previewToken.copy(asVisibleAsset: true),
      .previewDaiToken, // Verify non-visible assets not displayed #6386
      .mockUSDCToken.copy(asVisibleAsset: true),
      .mockERC721NFTToken
    ]
    let ethBalanceWei = formatter.weiString(
      from: mockETHBalance,
      radix: .hex,
      decimals: Int(BraveWallet.BlockchainToken.previewToken.decimals)
    ) ?? ""
    let usdcBalanceWei = formatter.weiString(
      from: mockUSDCBalance,
      radix: .hex,
      decimals: Int(BraveWallet.BlockchainToken.mockUSDCToken.decimals)
    ) ?? ""
    let mockNFTBalanceWei = formatter.weiString(from: 1, radix: .hex, decimals: 0) ?? ""
    let mockETHAssetPrice: BraveWallet.AssetPrice = .init(fromAsset: "eth", toAsset: "usd", price: mockETHPrice, assetTimeframeChange: "-57.23")
    let mockETHPriceHistory: [BraveWallet.AssetTimePrice] = [
      .init(date: Date(timeIntervalSinceNow: -1000), price: "3000.00"),
      .init(date: Date(), price: mockETHPrice)
    ]
    let mockUSDCAssetPrice: BraveWallet.AssetPrice = .init(
      fromAsset: BraveWallet.BlockchainToken.mockUSDCToken.assetRatioId,
      toAsset: "usd", price: mockUSDCPrice, assetTimeframeChange: "-57.23")
    let mockUSDCPriceHistory: [BraveWallet.AssetTimePrice] = [
      .init(date: Date(timeIntervalSinceNow: -1000), price: "0.999"),
      .init(date: Date(), price: mockUSDCPrice)
    ]
    let totalEthBalanceValue: Double = (Double(mockETHAssetPrice.price) ?? 0) * mockETHBalance
    let totalUSDCBalanceValue: Double = (Double(mockUSDCAssetPrice.price) ?? 0) * mockUSDCBalance
    
    let totalBalanceValue = totalEthBalanceValue + totalSolBalanceValue + totalUSDCBalanceValue
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
      case .btc:
        XCTFail("Should not fetch bitcoin network")
      @unknown default:
        XCTFail("Should not fetch unknown network")
      }
    }
    rpcService._balance = { _, _, _, completion in
      completion(ethBalanceWei, .success, "") // eth balance
    }
    rpcService._erc20TokenBalance = { contractAddress, _, _, completion in
      completion(usdcBalanceWei, .success, "") // usdc balance
    }
    rpcService._erc721TokenBalance = { contractAddress, _, _, _, completion in
      completion(mockNFTBalanceWei, .success, "") // eth nft balance
    }
    rpcService._solanaBalance = { accountAddress, chainId, completion in
      completion(mockSOLLamportBalance, .success, "") // sol balance
    }
    rpcService._splTokenAccountBalance = {_, _, _, completion in
      completion("\(mockNFTBalance)", UInt8(0), "\(mockNFTBalance)", .success, "") // sol nft balance
    }
    rpcService._erc721Metadata = { _, _, _, completion in
      let metadata = """
      {
        "image": "mock.image.url",
        "name": "mock nft name",
        "description": "mock nft description"
      }
      """
      completion("", metadata, .success, "")
    }
    rpcService._solTokenMetadata = { _, _, completion in
      let metaData = """
      {
        "image": "sol.mock.image.url",
        "name": "sol mock nft name",
        "description": "sol mock nft description"
      }
      """
      completion("", metaData, .success, "")
    }
    let walletService = BraveWallet.TestBraveWalletService()
    walletService._addObserver = { _ in }
    walletService._defaultBaseCurrency = { $0(CurrencyCode.usd.code) }
    walletService._selectedCoin = { $0(BraveWallet.CoinType.eth) }
    let assetRatioService = BraveWallet.TestAssetRatioService()
    assetRatioService._price = { priceIds, _, _, completion in
      completion(true, [mockETHAssetPrice, mockUSDCAssetPrice, mockSOLAssetPrice])
    }
    assetRatioService._priceHistory = { priceId, _, _, completion in
      switch priceId {
      case "sol":
        completion(true, mockSOLPriceHistory)
      case "eth":
        completion(true, mockETHPriceHistory)
      case BraveWallet.BlockchainToken.mockUSDCToken.assetRatioId:
        completion(true, mockUSDCPriceHistory)
      default:
        completion(false, [])
      }
    }
    
    let mockAssetManager = TestableWalletUserAssetManager()
    mockAssetManager._getAllVisibleAssetsInNetworkAssets = { _ in
      [NetworkAssets(network: .mockMainnet, tokens: mockEthUserAssets.filter({ $0.visible == true }), sortOrder: 0),
       NetworkAssets(network: .mockSolana, tokens: mockSolUserAssets.filter({ $0.visible == true }), sortOrder: 1)
      ]
    }
    
    // setup store
    let store = PortfolioStore(
      keyringService: keyringService,
      rpcService: rpcService,
      walletService: walletService,
      assetRatioService: assetRatioService,
      blockchainRegistry: BraveWallet.TestBlockchainRegistry(),
      ipfsApi: TestIpfsAPI(),
      userAssetManager: mockAssetManager
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
        // ETH on Ethereum mainnet, SOL on Solana mainnet, USDC on Ethereum mainnet
        XCTAssertEqual(lastUpdatedVisibleAssets.count, 3)
        // ETH
        XCTAssertEqual(lastUpdatedVisibleAssets[safe: 0]?.token.symbol,
                       BraveWallet.BlockchainToken.previewToken.symbol)
        XCTAssertEqual(lastUpdatedVisibleAssets[safe: 0]?.price,
                       mockETHAssetPrice.price)
        XCTAssertEqual(lastUpdatedVisibleAssets[safe: 0]?.history,
                       mockETHPriceHistory)
        // SOL
        XCTAssertEqual(lastUpdatedVisibleAssets[safe: 1]?.token.symbol,
                       BraveWallet.BlockchainToken.mockSolToken.symbol)
        XCTAssertEqual(lastUpdatedVisibleAssets[safe: 1]?.price,
                       mockSOLAssetPrice.price)
        XCTAssertEqual(lastUpdatedVisibleAssets[safe: 1]?.history,
                       mockSOLPriceHistory)
        // USDC first with largest balance
        XCTAssertEqual(lastUpdatedVisibleAssets[safe: 2]?.token.symbol,
                       BraveWallet.BlockchainToken.mockUSDCToken.symbol)
        XCTAssertEqual(lastUpdatedVisibleAssets[safe: 2]?.price,
                       mockUSDCAssetPrice.price)
        XCTAssertEqual(lastUpdatedVisibleAssets[safe: 2]?.history,
                       mockUSDCPriceHistory)
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

extension BraveWallet.BlockchainToken {
  /// Returns a copy of the `BlockchainToken` with the given `visible` flag.
  func copy(asVisibleAsset isVisible: Bool) -> Self {
    (self.copy() as! Self).then {
      $0.visible = isVisible
    }
  }
}
