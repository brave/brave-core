// Copyright 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation
import XCTest
import Combine
import BraveCore
@testable import BraveWallet

class UserAssetsStoreTests: XCTestCase {
  
  private var cancellables: Set<AnyCancellable> = .init()
  
  let networks: [BraveWallet.CoinType: [BraveWallet.NetworkInfo]] = [
    .eth: [.mockMainnet],
    .sol: [.mockSolana],
    .fil: [.mockFilecoinMainnet]
  ]
  let tokenRegistry: [BraveWallet.CoinType: [BraveWallet.BlockchainToken]] = [
    .eth: [.mockUSDCToken],
    .sol: [.mockSpdToken],
    .fil: [.mockFilToken]
  ]
  
  private func setupServices() -> (BraveWallet.TestKeyringService, BraveWallet.TestJsonRpcService, BraveWallet.TestBlockchainRegistry, BraveWallet.TestAssetRatioService, BraveWallet.TestBraveWalletService) {
    let keyringService = BraveWallet.TestKeyringService()
    keyringService._addObserver = { _ in }
    
    let rpcService = BraveWallet.TestJsonRpcService()
    rpcService._addObserver = { _ in }
    rpcService._allNetworks = { coin, completion in
      completion(self.networks[coin] ?? [])
    }
    rpcService._erc721Metadata = { _, _, _, completion in
      completion("", "", .internalError, "")
    }
    
    let blockchainRegistry = BraveWallet.TestBlockchainRegistry()
    blockchainRegistry._allTokens = { chainId, coin, completion in
      completion(self.tokenRegistry[coin] ?? [])
    }
    
    let assetRatioService = BraveWallet.TestAssetRatioService()
    
    let walletService = BraveWallet.TestBraveWalletService()
    walletService._addObserver = { _ in }

    return (keyringService, rpcService, blockchainRegistry, assetRatioService, walletService)
  }
  
  func testUpdate() {
    let (keyringService, rpcService, blockchainRegistry, assetRatioService, walletService) = setupServices()
    let mockAssetManager = TestableWalletUserAssetManager()
    mockAssetManager._getAllUserAssetsInNetworkAssets = { networks, _ in
      var result: [NetworkAssets] = []
      for network in networks {
        if network.chainId == BraveWallet.MainnetChainId {
          result.append(
            NetworkAssets(
              network: .mockMainnet,
              tokens: [
                BraveWallet.NetworkInfo.mockMainnet.nativeToken.copy(asVisibleAsset: true),
                .mockERC721NFTToken.copy(asVisibleAsset: true),
                .mockUSDCToken
              ],
              sortOrder: 0)
          )
        } else if network.chainId == BraveWallet.SolanaMainnet {
          result.append(
            NetworkAssets(
              network: .mockSolana,
              tokens: [
                BraveWallet.NetworkInfo.mockSolana.nativeToken.copy(asVisibleAsset: true),
                .mockSolanaNFTToken.copy(asVisibleAsset: true),
                .mockSpdToken
              ],
              sortOrder: 1)
          )
        } else if network.chainId == BraveWallet.FilecoinMainnet {
          result.append(
            NetworkAssets(
              network: .mockFilecoinMainnet,
              tokens: [
                BraveWallet.NetworkInfo.mockFilecoinMainnet.nativeToken.copy(asVisibleAsset: true)
              ],
              sortOrder: 1)
          )
        }
      }
      return result
    }
    
    let userAssetsStore = UserAssetsStore(
      blockchainRegistry: blockchainRegistry,
      rpcService: rpcService,
      keyringService: keyringService,
      assetRatioService: assetRatioService,
      walletService: walletService,
      ipfsApi: TestIpfsAPI(),
      userAssetManager: mockAssetManager
    )
    
    let assetStoresException = expectation(description: "userAssetsStore-assetStores")
    userAssetsStore.$assetStores
      .dropFirst()
      .sink { assetStores in
        defer { assetStoresException.fulfill() }
        XCTAssertEqual(assetStores.count, 7)
        
        XCTAssertEqual(assetStores[0].token.symbol, BraveWallet.NetworkInfo.mockSolana.nativeToken.symbol)
        XCTAssertTrue(assetStores[0].token.visible)
        XCTAssertEqual(assetStores[0].network, BraveWallet.NetworkInfo.mockSolana)
        
        XCTAssertEqual(assetStores[1].token.symbol, BraveWallet.BlockchainToken.mockSolanaNFTToken.symbol)
        XCTAssertTrue(assetStores[1].token.visible)
        XCTAssertEqual(assetStores[1].network, BraveWallet.NetworkInfo.mockSolana)
        
        XCTAssertEqual(assetStores[2].token.symbol, BraveWallet.BlockchainToken.mockSpdToken.symbol)
        XCTAssertFalse(assetStores[2].token.visible)
        XCTAssertEqual(assetStores[2].network, BraveWallet.NetworkInfo.mockSolana)
        
        XCTAssertEqual(assetStores[3].token.symbol, BraveWallet.NetworkInfo.mockMainnet.nativeToken.symbol)
        XCTAssertTrue(assetStores[3].token.visible)
        XCTAssertEqual(assetStores[3].network, BraveWallet.NetworkInfo.mockMainnet)
        
        XCTAssertEqual(assetStores[4].token.symbol, BraveWallet.BlockchainToken.mockERC721NFTToken.symbol)
        XCTAssertTrue(assetStores[4].token.visible)
        XCTAssertEqual(assetStores[4].network, BraveWallet.NetworkInfo.mockMainnet)
        
        XCTAssertEqual(assetStores[5].token.symbol, BraveWallet.BlockchainToken.mockUSDCToken.symbol)
        XCTAssertFalse(assetStores[5].token.visible)
        XCTAssertEqual(assetStores[5].network, BraveWallet.NetworkInfo.mockMainnet)
        
        XCTAssertEqual(assetStores[6].token.symbol, BraveWallet.BlockchainToken.mockFilToken.symbol)
        XCTAssertTrue(assetStores[6].token.visible)
        XCTAssertEqual(assetStores[6].network, BraveWallet.NetworkInfo.mockFilecoinMainnet)
      }
      .store(in: &cancellables)
    
    userAssetsStore.update()
    
    waitForExpectations(timeout: 1) { error in
      XCTAssertNil(error)
    }
  }
  
  @MainActor func testUpdateWithNetworkFilter() async {
    let (keyringService, rpcService, blockchainRegistry, assetRatioService, walletService) = setupServices()
    
    let mockAssetManager = TestableWalletUserAssetManager()
    mockAssetManager._getAllUserAssetsInNetworkAssets = { networks, _ in
      var result: [NetworkAssets] = []
      for network in networks {
        if network.chainId == BraveWallet.MainnetChainId {
          result.append(
            NetworkAssets(
            network: .mockMainnet,
            tokens: [
              BraveWallet.NetworkInfo.mockMainnet.nativeToken.copy(asVisibleAsset: true),
              .mockERC721NFTToken.copy(asVisibleAsset: true),
              .mockUSDCToken
            ],
            sortOrder: 0)
          )
        } else if network.chainId == BraveWallet.SolanaMainnet {
          result.append(
            NetworkAssets(
              network: .mockSolana,
              tokens: [
                BraveWallet.NetworkInfo.mockSolana.nativeToken.copy(asVisibleAsset: true),
                .mockSolanaNFTToken.copy(asVisibleAsset: true),
                .mockSpdToken
              ],
              sortOrder: 1)
          )
        } else if network.chainId == BraveWallet.FilecoinMainnet {
          result.append(
            NetworkAssets(
              network: .mockFilecoinMainnet,
              tokens: [
                BraveWallet.NetworkInfo.mockFilecoinMainnet.nativeToken.copy(asVisibleAsset: true)
              ],
              sortOrder: 1)
          )
        }
      }
      return result
    }
    
    let userAssetsStore = UserAssetsStore(
      blockchainRegistry: blockchainRegistry,
      rpcService: rpcService,
      keyringService: keyringService,
      assetRatioService: assetRatioService,
      walletService: walletService,
      ipfsApi: TestIpfsAPI(),
      userAssetManager: mockAssetManager
    )
    
    // Initial update() with all networks
    let setupException = expectation(description: "setup")
    userAssetsStore.$assetStores
      .dropFirst() // initial
      .sink { userVisibleNFTs in
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
        
        XCTAssertEqual(assetStores[0].token.symbol, BraveWallet.NetworkInfo.mockMainnet.nativeToken.symbol)
        XCTAssertTrue(assetStores[0].token.visible)
        XCTAssertEqual(assetStores[0].network, BraveWallet.NetworkInfo.mockMainnet)
        
        XCTAssertEqual(assetStores[1].token.symbol, BraveWallet.BlockchainToken.mockERC721NFTToken.symbol)
        XCTAssertTrue(assetStores[1].token.visible)
        XCTAssertEqual(assetStores[1].network, BraveWallet.NetworkInfo.mockMainnet)
        
        XCTAssertEqual(assetStores[2].token.symbol, BraveWallet.BlockchainToken.mockUSDCToken.symbol)
        XCTAssertFalse(assetStores[2].token.visible)
        XCTAssertEqual(assetStores[2].network, BraveWallet.NetworkInfo.mockMainnet)
      }
      .store(in: &cancellables)
    
    // network filter assignment should call `update()` and update `assetStores`
    userAssetsStore.networkFilters = [.init(isSelected: true, model: .mockMainnet)]
    
    await fulfillment(of: [assetStoresException], timeout: 1)
  }
}
