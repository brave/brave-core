// Copyright 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import Combine
import XCTest
import BraveCore
@testable import BraveWallet

class NFTStoreTests: XCTestCase {
  
  private var cancellables: Set<AnyCancellable> = .init()
  
  func testUpdate() {
    let ethNetwork: BraveWallet.NetworkInfo = .mockMainnet
    let mockEthUserAssets: [BraveWallet.BlockchainToken] = [
      .previewToken.then { $0.visible = true },
      .mockUSDCToken.then { $0.visible = false }, // Verify non-visible assets not displayed #6386
      .mockERC721NFTToken
    ]
    
    let solNetwork: BraveWallet.NetworkInfo = .mockSolana
    let mockSolUserAssets: [BraveWallet.BlockchainToken] = [
      BraveWallet.NetworkInfo.mockSolana.nativeToken.then { $0.visible = true },
      .mockSpdToken.then { $0.visible = false }, // Verify non-visible assets not displayed #6386
      .mockSolanaNFTToken
    ]
    
    let mockERC721Metadata: NFTMetadata = .init(imageURLString: "mock.image.url", name: "mock nft name", description: "mock nft description")
    let mockSolMetadata: NFTMetadata = .init(imageURLString: "sol.mock.image.url", name: "sol mock nft name", description: "sol mock nft description")
    
    // setup test services
    let keyringService = BraveWallet.TestKeyringService()
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
        XCTFail("Should not fetch btc network")
      @unknown default:
        XCTFail("Should not fetch unknown network")
      }
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
      let metadata = """
      {
        "image": "sol.mock.image.url",
        "name": "sol mock nft name",
        "description": "sol mock nft description"
      }
      """
      completion("", metadata, .success, "")
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
      case .btc:
        XCTFail("Should not fetch btc assets")
      @unknown default:
        XCTFail("Should not fetch unknown assets")
      }
    }
    walletService._addObserver = { _ in }
    let assetRatioService = BraveWallet.TestAssetRatioService()
    
    // setup store
    let store = NFTStore(
      keyringService: keyringService,
      rpcService: rpcService,
      walletService: walletService,
      assetRatioService: assetRatioService,
      blockchainRegistry: BraveWallet.TestBlockchainRegistry(),
      ipfsApi: TestIpfsAPI()
    )
    
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
        
        XCTAssertEqual(lastUpdatedVisibleNFTs[1].token.symbol, mockEthUserAssets.last?.symbol)
        XCTAssertEqual(lastUpdatedVisibleNFTs[safe: 0]?.nftMetadata?.imageURLString, mockSolMetadata.imageURLString)
        XCTAssertEqual(lastUpdatedVisibleNFTs[safe: 0]?.nftMetadata?.name, mockSolMetadata.name)
        XCTAssertEqual(lastUpdatedVisibleNFTs[safe: 0]?.nftMetadata?.description, mockSolMetadata.description)
        XCTAssertEqual(lastUpdatedVisibleNFTs[safe: 1]?.nftMetadata?.imageURLString, mockERC721Metadata.imageURLString)
        XCTAssertEqual(lastUpdatedVisibleNFTs[safe: 1]?.nftMetadata?.name, mockERC721Metadata.name)
        XCTAssertEqual(lastUpdatedVisibleNFTs[safe: 1]?.nftMetadata?.description, mockERC721Metadata.description)
      }.store(in: &cancellables)
    
    store.update()
    waitForExpectations(timeout: 1) { error in
      XCTAssertNil(error)
    }
  }
  
  func testUpdateAfterNetworkFilter() {
    let ethNetwork: BraveWallet.NetworkInfo = .mockMainnet
    let mockEthUserAssets: [BraveWallet.BlockchainToken] = [
      .previewToken.then { $0.visible = true },
      .mockUSDCToken.then { $0.visible = false }, // Verify non-visible assets not displayed #6386
      .mockERC721NFTToken
    ]
    
    let solNetwork: BraveWallet.NetworkInfo = .mockSolana
    let mockSolUserAssets: [BraveWallet.BlockchainToken] = [
      BraveWallet.NetworkInfo.mockSolana.nativeToken.then { $0.visible = true },
      .mockSpdToken.then { $0.visible = false }, // Verify non-visible assets not displayed #6386
      .mockSolanaNFTToken
    ]
    
    let mockSolMetadata: NFTMetadata = .init(imageURLString: "sol.mock.image.url", name: "sol mock nft name", description: "sol mock nft description")
    
    // setup test services
    let keyringService = BraveWallet.TestKeyringService()
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
        XCTFail("Should not fetch btc network")
      @unknown default:
        XCTFail("Should not fetch unknown network")
      }
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
      let metadata = """
      {
        "image": "sol.mock.image.url",
        "name": "sol mock nft name",
        "description": "sol mock nft description"
      }
      """
      completion("", metadata, .success, "")
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
      case .btc:
        XCTFail("Should not fetch btc assets")
      @unknown default:
        XCTFail("Should not fetch unknown assets")
      }
    }
    walletService._addObserver = { _ in }
    let assetRatioService = BraveWallet.TestAssetRatioService()
    
    // setup store
    let store = NFTStore(
      keyringService: keyringService,
      rpcService: rpcService,
      walletService: walletService,
      assetRatioService: assetRatioService,
      blockchainRegistry: BraveWallet.TestBlockchainRegistry(),
      ipfsApi: TestIpfsAPI()
    )
    
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
        XCTAssertEqual(lastUpdatedVisibleNFTs.count, 1)
        XCTAssertEqual(lastUpdatedVisibleNFTs[0].token.symbol, mockSolUserAssets.last?.symbol)
        XCTAssertEqual(lastUpdatedVisibleNFTs[safe: 0]?.nftMetadata?.imageURLString, mockSolMetadata.imageURLString)
        XCTAssertEqual(lastUpdatedVisibleNFTs[safe: 0]?.nftMetadata?.name, mockSolMetadata.name)
        XCTAssertEqual(lastUpdatedVisibleNFTs[safe: 0]?.nftMetadata?.description, mockSolMetadata.description)
      }.store(in: &cancellables)
    
    store.networkFilter = .network(solNetwork)
    waitForExpectations(timeout: 1) { error in
      XCTAssertNil(error)
    }
  }
}
