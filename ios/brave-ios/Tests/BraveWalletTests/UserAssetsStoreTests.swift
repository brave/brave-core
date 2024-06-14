// Copyright 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveCore
import Combine
import Foundation
import XCTest

@testable import BraveWallet

class UserAssetsStoreTests: XCTestCase {

  private var cancellables: Set<AnyCancellable> = .init()

  let nonNativeTokenVisibleAssets: [BraveWallet.BlockchainToken] = [
    .mockERC721NFTToken.copy(asVisibleAsset: true),
    .mockSpdToken.copy(asVisibleAsset: true),
  ]
  let tokenRegistry: [BraveWallet.CoinType: [BraveWallet.BlockchainToken]] = [
    .eth: [.mockUSDCToken.copy(asVisibleAsset: false)]
  ]

  private func setupServices() -> (
    BraveWallet.TestKeyringService, BraveWallet.TestJsonRpcService,
    BraveWallet.TestBlockchainRegistry, BraveWallet.TestAssetRatioService,
    BraveWallet.TestBraveWalletService, WalletUserAssetManagerType
  ) {
    let keyringService = BraveWallet.TestKeyringService()
    keyringService._addObserver = { _ in }

    let rpcService = MockJsonRpcService()

    let blockchainRegistry = BraveWallet.TestBlockchainRegistry()
    blockchainRegistry._allTokens = { chainId, coin, completion in
      completion((self.tokenRegistry[coin] ?? []).filter { $0.chainId == chainId })
    }

    let assetRatioService = BraveWallet.TestAssetRatioService()

    let walletService = BraveWallet.TestBraveWalletService()
    walletService._addObserver = { _ in }

    let mockAssetManager = TestableWalletUserAssetManager()
    mockAssetManager._getAllUserAssetsInNetworkAssets = { networks, _ in
      var result: [NetworkAssets] = []
      for (index, network) in networks.enumerated() {
        result.append(
          NetworkAssets(
            network: network,
            tokens: [network.nativeToken.copy(asVisibleAsset: true)]
              + self.nonNativeTokenVisibleAssets.filter { $0.chainId == network.chainId },
            sortOrder: index
          )
        )
      }
      return result
    }

    return (
      keyringService, rpcService, blockchainRegistry, assetRatioService, walletService,
      mockAssetManager
    )
  }

  func testUpdate() {
    let (
      keyringService, rpcService, blockchainRegistry, assetRatioService, walletService, assetManager
    ) =
      setupServices()

    let userAssetsStore = UserAssetsStore(
      blockchainRegistry: blockchainRegistry,
      rpcService: rpcService,
      keyringService: keyringService,
      assetRatioService: assetRatioService,
      walletService: walletService,
      ipfsApi: TestIpfsAPI(),
      userAssetManager: assetManager
    )

    let assetStoresException = expectation(description: "userAssetsStore-assetStores")
    userAssetsStore.$assetStores
      .dropFirst()
      .sink { assetStores in
        defer { assetStoresException.fulfill() }
        XCTAssertEqual(assetStores.count, 8)

        XCTAssertEqual(
          assetStores[0].token.symbol,
          BraveWallet.NetworkInfo.mockSolana.nativeToken.symbol
        )
        XCTAssertTrue(assetStores[0].token.visible)
        XCTAssertEqual(assetStores[0].network, BraveWallet.NetworkInfo.mockSolana)

        XCTAssertEqual(assetStores[1].token.symbol, BraveWallet.BlockchainToken.mockSpdToken.symbol)
        XCTAssertTrue(assetStores[1].token.visible)
        XCTAssertEqual(assetStores[1].network, BraveWallet.NetworkInfo.mockSolana)

        XCTAssertEqual(
          assetStores[2].token.symbol,
          BraveWallet.NetworkInfo.mockMainnet.nativeToken.symbol
        )
        XCTAssertTrue(assetStores[2].token.visible)
        XCTAssertEqual(assetStores[2].network, BraveWallet.NetworkInfo.mockMainnet)

        XCTAssertEqual(
          assetStores[3].token.symbol,
          BraveWallet.BlockchainToken.mockERC721NFTToken.symbol
        )
        XCTAssertTrue(assetStores[3].token.visible)
        XCTAssertEqual(assetStores[3].network, BraveWallet.NetworkInfo.mockMainnet)

        XCTAssertEqual(
          assetStores[4].token.symbol,
          BraveWallet.BlockchainToken.mockUSDCToken.symbol
        )
        XCTAssertFalse(assetStores[4].token.visible)
        XCTAssertEqual(assetStores[4].network, BraveWallet.NetworkInfo.mockMainnet)

        XCTAssertEqual(
          assetStores[5].token.symbol,
          BraveWallet.NetworkInfo.mockPolygon.nativeToken.symbol
        )
        XCTAssertTrue(assetStores[5].token.visible)
        XCTAssertEqual(assetStores[5].network, BraveWallet.NetworkInfo.mockPolygon)

        XCTAssertEqual(
          assetStores[6].token.symbol,
          BraveWallet.NetworkInfo.mockFilecoinMainnet.nativeToken.symbol
        )
        XCTAssertTrue(assetStores[6].token.visible)
        XCTAssertEqual(assetStores[6].network, BraveWallet.NetworkInfo.mockFilecoinMainnet)

        XCTAssertEqual(
          assetStores[7].token.symbol,
          BraveWallet.NetworkInfo.mockBitcoinMainnet.nativeToken.symbol
        )
        XCTAssertTrue(assetStores[7].token.visible)
        XCTAssertEqual(assetStores[7].network, BraveWallet.NetworkInfo.mockBitcoinMainnet)
      }
      .store(in: &cancellables)

    userAssetsStore.update()

    waitForExpectations(timeout: 1) { error in
      XCTAssertNil(error)
    }
  }

  @MainActor func testUpdateWithNetworkFilter() async {
    let (
      keyringService, rpcService, blockchainRegistry, assetRatioService, walletService, assetManager
    ) =
      setupServices()

    let userAssetsStore = UserAssetsStore(
      blockchainRegistry: blockchainRegistry,
      rpcService: rpcService,
      keyringService: keyringService,
      assetRatioService: assetRatioService,
      walletService: walletService,
      ipfsApi: TestIpfsAPI(),
      userAssetManager: assetManager
    )

    // Initial update() with all networks
    let setupException = expectation(description: "setup")
    userAssetsStore.$assetStores
      .dropFirst()  // initial
      .sink { _ in
        setupException.fulfill()
      }.store(in: &cancellables)
    userAssetsStore.update()
    await fulfillment(of: [setupException], timeout: 1)
    cancellables.removeAll()

    let assetStoresException = expectation(description: "userAssetsStore-assetStores")
    userAssetsStore.$assetStores
      .dropFirst()
      .sink { assetStores in
        defer { assetStoresException.fulfill() }
        XCTAssertEqual(assetStores.count, 3)

        XCTAssertEqual(
          assetStores[0].token.symbol,
          BraveWallet.NetworkInfo.mockMainnet.nativeToken.symbol
        )
        XCTAssertTrue(assetStores[0].token.visible)
        XCTAssertEqual(assetStores[0].network, BraveWallet.NetworkInfo.mockMainnet)

        XCTAssertEqual(
          assetStores[1].token.symbol,
          BraveWallet.BlockchainToken.mockERC721NFTToken.symbol
        )
        XCTAssertTrue(assetStores[1].token.visible)
        XCTAssertEqual(assetStores[1].network, BraveWallet.NetworkInfo.mockMainnet)

        XCTAssertEqual(
          assetStores[2].token.symbol,
          BraveWallet.BlockchainToken.mockUSDCToken.symbol
        )
        XCTAssertFalse(assetStores[2].token.visible)
        XCTAssertEqual(assetStores[2].network, BraveWallet.NetworkInfo.mockMainnet)
      }
      .store(in: &cancellables)

    // network filter assignment should call `update()` and update `assetStores`
    userAssetsStore.networkFilters = [.init(isSelected: true, model: .mockMainnet)]

    await fulfillment(of: [assetStoresException], timeout: 1)
  }
}
