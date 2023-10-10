// Copyright 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import Combine
import XCTest
import BraveCore
import Preferences
@testable import BraveWallet

class NFTStoreTests: XCTestCase {
  
  private var cancellables: Set<AnyCancellable> = .init()
  
  let ethNetwork: BraveWallet.NetworkInfo = .mockMainnet
  let ethAccount1: BraveWallet.AccountInfo = .mockEthAccount
  let ethAccount2 = (BraveWallet.AccountInfo.mockEthAccount.copy() as! BraveWallet.AccountInfo).then {
    $0.address = "mock_eth_id_2"
    $0.name = "Ethereum Account 2"
  }
  let unownedEthNFT = (BraveWallet.BlockchainToken.mockERC721NFTToken.copy() as! BraveWallet.BlockchainToken).then {
    $0.contractAddress = "0xbbbbbbbbbb222222222233333333334444444444"
    $0.name = "Unowned NFT"
  }
  let invisibleEthNFT = (BraveWallet.BlockchainToken.mockERC721NFTToken.copy() as! BraveWallet.BlockchainToken).then {
    $0.contractAddress = "0xbbbbbbbbbb222222222233333333335555555555"
    $0.name = "Invisible NFT"
    $0.visible = false
  }
  
  let solNetwork: BraveWallet.NetworkInfo = .mockSolana
  let solAccount: BraveWallet.AccountInfo = .mockSolAccount
  
  let mockERC721Metadata: NFTMetadata = .init(imageURLString: "mock.image.url", name: "mock nft name", description: "mock nft description")
  let mockSolMetadata: NFTMetadata = .init(imageURLString: "sol.mock.image.url", name: "sol mock nft name", description: "sol mock nft description")
  
  let spamEthNFT = (BraveWallet.BlockchainToken.mockERC721NFTToken.copy() as! BraveWallet.BlockchainToken).then {
    $0.contractAddress = "0xbbbbbbbbbb222222222233333333336666666666"
    $0.name = "Spam Eth NFT"
    $0.visible = false
    $0.isSpam = true
  }
  
  private func setupServices(
    mockEthUserAssets: [BraveWallet.BlockchainToken],
    mockSolUserAssets: [BraveWallet.BlockchainToken]
  ) -> (BraveWallet.TestKeyringService, BraveWallet.TestJsonRpcService, BraveWallet.TestBraveWalletService, BraveWallet.TestAssetRatioService, TestableWalletUserAssetManager) {
    let keyringService = BraveWallet.TestKeyringService()
    keyringService._addObserver = { _ in }
    keyringService._isLocked = { completion in
      // unlocked would cause `update()` from call in `init` to be called prior to test being setup.g
      completion(true)
    }
    keyringService._allAccounts = {
      $0(.init(
        accounts: [self.solAccount, self.ethAccount1, self.ethAccount2],
        selectedAccount: self.ethAccount1,
        ethDappSelectedAccount: self.ethAccount1,
        solDappSelectedAccount: self.solAccount
      ))
    }
    
    let rpcService = BraveWallet.TestJsonRpcService()
    rpcService._addObserver = { _ in }
    rpcService._allNetworks = { coin, completion in
      switch coin {
      case .eth:
        completion([self.ethNetwork])
      case .sol:
        completion([self.solNetwork])
      case .fil:
        completion([.mockFilecoinTestnet])
      case .btc:
        XCTFail("Should not fetch btc network")
      @unknown default:
        XCTFail("Should not fetch unknown network")
      }
    }
    rpcService._erc721Metadata = { contractAddress, tokenId, chainId, completion in
      guard contractAddress == BraveWallet.BlockchainToken.mockERC721NFTToken.contractAddress else {
        completion("", "", .internalError, "Error")
        return
      }
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
    rpcService._erc721TokenBalance = { contractAddress, tokenId, accountAddress, chainId, completion in
      guard accountAddress == self.ethAccount1.address,
            contractAddress == BraveWallet.BlockchainToken.mockERC721NFTToken.contractAddress else {
        completion("", .internalError, "Error")
        return
      }
      completion("0x1", .success, "")
    }
    rpcService._splTokenAccountBalance = { accountAddress, tokenMintAddress, chainId, completion in
      guard accountAddress == self.solAccount.address,
            tokenMintAddress == mockSolUserAssets[safe: 2]?.contractAddress else {
        completion("", 0, "", .internalError, "Error")
        return
      }
      completion("1", 0, "1", .success, "")
    }
    let walletService = BraveWallet.TestBraveWalletService()
    walletService._addObserver = { _ in }
    walletService._simpleHashSpamNfTs = {walletAddress, chainIds, coin, _, completion in
      if walletAddress == self.ethAccount1.address, chainIds.contains(BraveWallet.MainnetChainId), coin == .eth {
        completion([self.spamEthNFT], nil)
      } else {
        completion([], nil)
      }
    }
    let assetRatioService = BraveWallet.TestAssetRatioService()
    
    let mockAssetManager = TestableWalletUserAssetManager()
    mockAssetManager._getAllUserAssetsInNetworkAssetsByVisibility = { networks, visible in
      [
        NetworkAssets(
          network: .mockMainnet,
          tokens: mockEthUserAssets.filter({ $0.visible == visible && $0.isSpam == false }),
          sortOrder: 0
        ),
        NetworkAssets(
          network: .mockSolana,
          tokens: mockSolUserAssets.filter({ $0.visible == visible && $0.isSpam == false }),
          sortOrder: 1
        )
      ].filter { networkAsset in networks.contains(where: { $0 == networkAsset.network }) }
    }
    mockAssetManager._getAllUserNFTs = { networks, isSpam in
      [
        NetworkAssets(
          network: .mockSolana,
          tokens: mockSolUserAssets.filter({ $0.isSpam == isSpam }),
          sortOrder: 0
        )
      ].filter { networkAsset in networks.contains(where: { $0 == networkAsset.network }) }
    }
    
    return (keyringService, rpcService, walletService, assetRatioService, mockAssetManager)
  }
  
  override func setUp() {
    resetFilters()
  }
  override func tearDown() {
    resetFilters()
  }
  private func resetFilters() {
    Preferences.Wallet.isHidingUnownedNFTsFilter.reset()
    Preferences.Wallet.isShowingNFTNetworkLogoFilter.reset()
    Preferences.Wallet.nonSelectedAccountsFilter.reset()
    Preferences.Wallet.nonSelectedNetworksFilter.reset()
  }
  
  func testUpdate() async {
    let mockEthUserAssets: [BraveWallet.BlockchainToken] = [
      .previewToken.copy(asVisibleAsset: true),
      .mockUSDCToken.copy(asVisibleAsset: false), // Verify non-visible assets not displayed #6386
      .mockERC721NFTToken,
      unownedEthNFT,
      invisibleEthNFT
    ]
    let mockSolUserAssets: [BraveWallet.BlockchainToken] = [
      BraveWallet.NetworkInfo.mockSolana.nativeToken.copy(asVisibleAsset: true),
      .mockSpdToken.copy(asVisibleAsset: false), // Verify non-visible assets not displayed #6386
      .mockSolanaNFTToken
    ]
    
    // setup test services
    let (keyringService, rpcService, walletService, assetRatioService, mockAssetManager) = setupServices(mockEthUserAssets: mockEthUserAssets, mockSolUserAssets: mockSolUserAssets)
    
    // setup store
    let store = NFTStore(
      keyringService: keyringService,
      rpcService: rpcService,
      walletService: walletService,
      assetRatioService: assetRatioService,
      blockchainRegistry: BraveWallet.TestBlockchainRegistry(),
      ipfsApi: TestIpfsAPI(),
      userAssetManager: mockAssetManager
    )
    
    // test that `update()` will assign new value to `userNFTs` publisher
    let userVisibleNFTsException = expectation(description: "update-userVisibleNFTs")
    XCTAssertTrue(store.userNFTs.isEmpty)  // Initial state
    store.$userNFTs
      .dropFirst()
      .collect(3)
      .sink { userNFTs in
        defer { userVisibleNFTsException.fulfill() }
        XCTAssertEqual(userNFTs.count, 3) // empty nfts, populated w/ balance nfts, populated w/ metadata
        guard let lastUpdatedUserNFTs = userNFTs.last else {
          XCTFail("Unexpected test result")
          return
        }
        let visibleNFTs = lastUpdatedUserNFTs.filter(\.token.visible)
        XCTAssertEqual(visibleNFTs.count, 3)
        XCTAssertEqual(visibleNFTs[safe: 0]?.token.symbol, mockEthUserAssets[safe: 2]?.symbol)
        XCTAssertEqual(visibleNFTs[safe: 0]?.nftMetadata?.imageURLString, self.mockERC721Metadata.imageURLString)
        XCTAssertEqual(visibleNFTs[safe: 0]?.nftMetadata?.name, self.mockERC721Metadata.name)
        XCTAssertEqual(visibleNFTs[safe: 0]?.nftMetadata?.description, self.mockERC721Metadata.description)
        XCTAssertEqual(visibleNFTs[safe: 1]?.token.symbol, mockEthUserAssets[safe: 3]?.symbol)
        XCTAssertNil(visibleNFTs[safe: 1]?.nftMetadata)
        
        XCTAssertEqual(visibleNFTs[safe: 2]?.token.symbol, mockSolUserAssets[safe: 2]?.symbol)
        XCTAssertEqual(visibleNFTs[safe: 2]?.nftMetadata?.imageURLString, self.mockSolMetadata.imageURLString)
        XCTAssertEqual(visibleNFTs[safe: 2]?.nftMetadata?.name, self.mockSolMetadata.name)
        XCTAssertEqual(visibleNFTs[safe: 2]?.nftMetadata?.description, self.mockSolMetadata.description)
      }.store(in: &cancellables)
    
    store.update()
    await fulfillment(of: [userVisibleNFTsException], timeout: 1)
    cancellables.removeAll()
    
    let defaultFilters = store.filters
    
    // MARK: Network Filter Test
    let networksExpectation = expectation(description: "update-networks")
    store.$userNFTs
      .dropFirst()
      .collect(2)
      .sink { userNFTs in
        defer { networksExpectation.fulfill() }
        XCTAssertEqual(userNFTs.count, 2) // empty nfts, populated nfts
        guard let lastUpdatedUserNFTs = userNFTs.last else {
          XCTFail("Unexpected test result")
          return
        }
        let visibleNFTs = lastUpdatedUserNFTs.filter(\.token.visible)
        XCTAssertEqual(visibleNFTs.count, 2)
        XCTAssertEqual(visibleNFTs[safe: 0]?.token.symbol, mockEthUserAssets[safe: 2]?.symbol)
        XCTAssertEqual(visibleNFTs[safe: 1]?.token.symbol, mockEthUserAssets[safe: 3]?.symbol)
        // solana NFT hidden
      }.store(in: &cancellables)
    store.saveFilters(.init(
      groupBy: defaultFilters.groupBy,
      sortOrder: defaultFilters.sortOrder,
      isHidingSmallBalances: defaultFilters.isHidingSmallBalances,
      isHidingUnownedNFTs: defaultFilters.isHidingUnownedNFTs,
      isShowingNFTNetworkLogo: defaultFilters.isShowingNFTNetworkLogo,
      accounts: defaultFilters.accounts,
      networks: defaultFilters.networks.map {
        .init(isSelected: $0.model.coin == .eth, model: $0.model)
      }
    ))
    await fulfillment(of: [networksExpectation], timeout: 1)
    cancellables.removeAll()
    
    // MARK: Hiding Unowned Filter Test
    let hidingUnownedExpectation = expectation(description: "update-hidingUnowned")
    store.$userNFTs
      .dropFirst()
      .collect(3)
      .sink { userNFTs in
        defer { hidingUnownedExpectation.fulfill() }
        XCTAssertEqual(userNFTs.count, 3) // empty nfts, populated w/ balance nfts, populated w/ metadata
        guard let lastUpdatedUserNFTs = userNFTs.last else {
          XCTFail("Unexpected test result")
          return
        }
        let visibleNFTs = lastUpdatedUserNFTs.filter(\.token.visible)
        XCTAssertEqual(visibleNFTs.count, 2)
        XCTAssertEqual(visibleNFTs[safe: 0]?.token.symbol, mockEthUserAssets[safe: 2]?.symbol)
        XCTAssertEqual(visibleNFTs[safe: 1]?.token.symbol, mockSolUserAssets[safe: 2]?.symbol)
      }.store(in: &cancellables)
    store.saveFilters(.init(
      groupBy: defaultFilters.groupBy,
      sortOrder: defaultFilters.sortOrder,
      isHidingSmallBalances: defaultFilters.isHidingSmallBalances,
      isHidingUnownedNFTs: true,
      isShowingNFTNetworkLogo: defaultFilters.isShowingNFTNetworkLogo,
      accounts: defaultFilters.accounts,
      networks: defaultFilters.networks
    ))
    await fulfillment(of: [hidingUnownedExpectation], timeout: 1)
    cancellables.removeAll()
    
    // MARK: Accounts Filter Test
    let accountsExpectation = expectation(description: "update-accounts")
    store.$userNFTs
      .dropFirst()
      .collect(2)
      .sink { userNFTs in
        defer { accountsExpectation.fulfill() }
        XCTAssertEqual(userNFTs.count, 2) // empty nfts, populated nfts
        guard let lastUpdatedNFTs = userNFTs.last else {
          XCTFail("Unexpected test result")
          return
        }
        let visibleNFTs = lastUpdatedNFTs.filter(\.token.visible)
        XCTAssertEqual(visibleNFTs.count, 1)
        XCTAssertEqual(visibleNFTs[safe: 0]?.token.symbol, mockEthUserAssets[safe: 2]?.symbol)
      }.store(in: &cancellables)
    store.saveFilters(.init(
      groupBy: defaultFilters.groupBy,
      sortOrder: defaultFilters.sortOrder,
      isHidingSmallBalances: defaultFilters.isHidingSmallBalances,
      isHidingUnownedNFTs: true,
      isShowingNFTNetworkLogo: defaultFilters.isShowingNFTNetworkLogo,
      accounts: defaultFilters.accounts.map { // only ethereum accounts selected
        .init(isSelected: $0.model.coin == .eth, model: $0.model)
      },
      networks: defaultFilters.networks
    ))
    await fulfillment(of: [accountsExpectation], timeout: 1)
    cancellables.removeAll()
  }
  
  func testUpdateForInvisibleGroup() async {
    let mockEthUserAssets: [BraveWallet.BlockchainToken] = [
      .previewToken.copy(asVisibleAsset: true),
      .mockUSDCToken.copy(asVisibleAsset: false), // Verify non-visible assets not displayed #6386
      .mockERC721NFTToken,
      unownedEthNFT,
      invisibleEthNFT
    ]
    let mockSolUserAssets: [BraveWallet.BlockchainToken] = [
      BraveWallet.NetworkInfo.mockSolana.nativeToken.copy(asVisibleAsset: true),
      .mockSpdToken.copy(asVisibleAsset: false), // Verify non-visible assets not displayed #6386
      .mockSolanaNFTToken
    ]
    
    // setup test services
    let (keyringService, rpcService, walletService, assetRatioService, mockAssetManager) = setupServices(mockEthUserAssets: mockEthUserAssets, mockSolUserAssets: mockSolUserAssets)
    rpcService._erc721Metadata = { contractAddress, tokenId, chainId, completion in
      let metadata = """
      {
        "image": "mock.image.url",
        "name": "mock nft name",
        "description": "mock nft description"
      }
      """
      completion("", metadata, .success, "")
    }
    
    // setup store
    let store = NFTStore(
      keyringService: keyringService,
      rpcService: rpcService,
      walletService: walletService,
      assetRatioService: assetRatioService,
      blockchainRegistry: BraveWallet.TestBlockchainRegistry(),
      ipfsApi: TestIpfsAPI(),
      userAssetManager: mockAssetManager
    )
    
    // test that `update()` will assign new value to `userInvisibleNFTs` publisher
    let userHiddenNFTsException = expectation(description: "update-userInvisibleNFTs")
    store.$userNFTs
      .dropFirst()
      .collect(3)
      .sink { userNFTs in
        defer { userHiddenNFTsException.fulfill() }
        XCTAssertEqual(userNFTs.count, 3) // empty nfts, populated w/ balance nfts, populated w/ metadata
        guard let lastUpdatedNFTs = userNFTs.last else {
          XCTFail("Unexpected test result")
          return
        }
        let hiddenNFTs = lastUpdatedNFTs.filter { !$0.token.visible && !$0.token.isSpam }
        XCTAssertEqual(hiddenNFTs.count, 1)
        XCTAssertEqual(hiddenNFTs[safe: 0]?.token.symbol, mockEthUserAssets[safe: 4]?.symbol)
        XCTAssertEqual(hiddenNFTs[safe: 0]?.nftMetadata?.imageURLString, self.mockERC721Metadata.imageURLString)
        XCTAssertEqual(hiddenNFTs[safe: 0]?.nftMetadata?.name, self.mockERC721Metadata.name)
      }.store(in: &cancellables)
    store.update()
    await fulfillment(of: [userHiddenNFTsException], timeout: 1)
    cancellables.removeAll()
  }
  
  func testUpdateSpamGrupOnlyFromSimpleHash() async {
    let mockEthUserAssets: [BraveWallet.BlockchainToken] = [
      .previewToken.copy(asVisibleAsset: true),
      .mockUSDCToken.copy(asVisibleAsset: false), // Verify non-visible assets not displayed #6386
      .mockERC721NFTToken,
      unownedEthNFT,
      invisibleEthNFT
    ]
    let mockSolUserAssets: [BraveWallet.BlockchainToken] = [
      BraveWallet.NetworkInfo.mockSolana.nativeToken.copy(asVisibleAsset: true),
      .mockSpdToken.copy(asVisibleAsset: false), // Verify non-visible assets not displayed #6386
      .mockSolanaNFTToken
    ]

    // setup test services
    let (keyringService, rpcService, walletService, assetRatioService, mockAssetManager) = setupServices(mockEthUserAssets: mockEthUserAssets, mockSolUserAssets: mockSolUserAssets)
    rpcService._erc721Metadata = { contractAddress, tokenId, chainId, completion in
      let metadata = """
      {
        "image": "mock.image.url",
        "name": "mock nft name",
        "description": "mock nft description"
      }
      """
      completion("", metadata, .success, "")
    }

    // setup store
    let store = NFTStore(
      keyringService: keyringService,
      rpcService: rpcService,
      walletService: walletService,
      assetRatioService: assetRatioService,
      blockchainRegistry: BraveWallet.TestBlockchainRegistry(),
      ipfsApi: TestIpfsAPI(),
      userAssetManager: mockAssetManager
    )

    // test that `update()` will assign new value to `userNFTs` publisher
    let userSpamNFTsException = expectation(description: "update-userInvisibleNFTs1")
    store.$userNFTs
      .dropFirst()
      .collect(3)
      .sink { userNFTs in
        defer { userSpamNFTsException.fulfill() }
        XCTAssertEqual(userNFTs.count, 3) // empty nfts, populated w/ balance nfts, populated w/ metadata
        guard let lastUpdatedUserNFTs = userNFTs.last else {
          XCTFail("Unexpected test result")
          return
        }
        let spamNFTs = lastUpdatedUserNFTs.filter(\.token.isSpam)
        XCTAssertEqual(spamNFTs.count, 1)
        XCTAssertEqual(spamNFTs[safe: 0]?.token.symbol, self.spamEthNFT.symbol)
        XCTAssertEqual(spamNFTs[safe: 0]?.nftMetadata?.imageURLString, self.mockERC721Metadata.imageURLString)
        XCTAssertEqual(spamNFTs[safe: 0]?.nftMetadata?.name, self.mockERC721Metadata.name)
      }.store(in: &cancellables)

    store.update()
    await fulfillment(of: [userSpamNFTsException], timeout: 1)
    cancellables.removeAll()
  }
  
  func testUpdateSpamGrupFromSimpleHashAndUserMarked() async {
    let mockEthUserAssets: [BraveWallet.BlockchainToken] = [
      .previewToken.copy(asVisibleAsset: true),
      .mockUSDCToken.copy(asVisibleAsset: false), // Verify non-visible assets not displayed #6386
      .mockERC721NFTToken,
      invisibleEthNFT
    ]
    let mockSolUserAssets: [BraveWallet.BlockchainToken] = [
      BraveWallet.NetworkInfo.mockSolana.nativeToken.copy(asVisibleAsset: true),
      .mockSpdToken.copy(asVisibleAsset: false), // Verify non-visible assets not displayed #6386
      .mockSolanaNFTToken.copy(asVisibleAsset: false, isSpam: true)
    ]
    
    // setup test services
    let (keyringService, rpcService, walletService, assetRatioService, mockAssetManager) = setupServices(mockEthUserAssets: mockEthUserAssets, mockSolUserAssets: mockSolUserAssets)
    rpcService._erc721Metadata = { contractAddress, tokenId, chainId, completion in
      let metadata = """
      {
        "image": "mock.image.url",
        "name": "mock nft name",
        "description": "mock nft description"
      }
      """
      completion("", metadata, .success, "")
    }
    
    
    // setup store
    let store = NFTStore(
      keyringService: keyringService,
      rpcService: rpcService,
      walletService: walletService,
      assetRatioService: assetRatioService,
      blockchainRegistry: BraveWallet.TestBlockchainRegistry(),
      ipfsApi: TestIpfsAPI(),
      userAssetManager: mockAssetManager
    )
    
    // test that `update()` will assign new value to `userNFTs` publisher
    let userSpamNFTsException = expectation(description: "update-userSpamNFTsException2")
    store.$userNFTs
      .dropFirst()
      .collect(3)
      .sink { userNFTs in
        defer { userSpamNFTsException.fulfill() }
        XCTAssertEqual(userNFTs.count, 3) // empty nfts, populated w/ balance nfts, populated w/ metadata
        guard let lastUpdatedUserNFTs = userNFTs.last else {
          XCTFail("Unexpected test result")
          return
        }
        let spamNFTs = lastUpdatedUserNFTs.filter(\.token.isSpam)
        XCTAssertEqual(spamNFTs.count, 2)
        XCTAssertEqual(spamNFTs[safe: 0]?.token.symbol, BraveWallet.BlockchainToken.mockSolanaNFTToken.symbol)
        XCTAssertEqual(spamNFTs[safe: 0]?.nftMetadata?.imageURLString, self.mockSolMetadata.imageURLString)
        XCTAssertEqual(spamNFTs[safe: 0]?.nftMetadata?.name, self.mockSolMetadata.name)
        XCTAssertEqual(spamNFTs[safe: 1]?.token.symbol, self.spamEthNFT.symbol)
        XCTAssertEqual(spamNFTs[safe: 1]?.nftMetadata?.imageURLString, self.mockERC721Metadata.imageURLString)
        XCTAssertEqual(spamNFTs[safe: 1]?.nftMetadata?.name, self.mockERC721Metadata.name)
      }.store(in: &cancellables)
    
    store.update()
    await fulfillment(of: [userSpamNFTsException], timeout: 1)
    cancellables.removeAll()
  }
  
  func testUpdateSpamGrupDuplicationFromSimpleHashAndUserMarked() async {
    let mockEthUserAssets: [BraveWallet.BlockchainToken] = [
      .previewToken.copy(asVisibleAsset: true),
      .mockUSDCToken.copy(asVisibleAsset: false), // Verify non-visible assets not displayed #6386
      .mockERC721NFTToken,
      invisibleEthNFT,
      spamEthNFT
    ]
    let mockSolUserAssets: [BraveWallet.BlockchainToken] = [
      BraveWallet.NetworkInfo.mockSolana.nativeToken.copy(asVisibleAsset: true),
      .mockSpdToken.copy(asVisibleAsset: false), // Verify non-visible assets not displayed #6386
      .mockSolanaNFTToken
    ]
    
    // setup test services
    let (keyringService, rpcService, walletService, assetRatioService, mockAssetManager) = setupServices(mockEthUserAssets: mockEthUserAssets, mockSolUserAssets: mockSolUserAssets)
    rpcService._erc721Metadata = { contractAddress, tokenId, chainId, completion in
      let metadata = """
      {
        "image": "mock.image.url",
        "name": "mock nft name",
        "description": "mock nft description"
      }
      """
      completion("", metadata, .success, "")
    }
    
    
    // setup store
    let store = NFTStore(
      keyringService: keyringService,
      rpcService: rpcService,
      walletService: walletService,
      assetRatioService: assetRatioService,
      blockchainRegistry: BraveWallet.TestBlockchainRegistry(),
      ipfsApi: TestIpfsAPI(),
      userAssetManager: mockAssetManager
    )
    
    // test that `update()` will assign new value to `userSpamNFTs` publisher
    let userSpamNFTsException = expectation(description: "update-userSpamNFTsException2")
    store.$userNFTs
      .dropFirst()
      .collect(3)
      .sink { userNFTs in
        defer { userSpamNFTsException.fulfill() }
        XCTAssertEqual(userNFTs.count, 3) // empty nfts, populated w/ balance nfts, populated w/ metadata
        guard let lastUpdatedUserNFTs = userNFTs.last else {
          XCTFail("Unexpected test result")
          return
        }
        let spamNFTs = lastUpdatedUserNFTs.filter(\.token.isSpam)
        XCTAssertEqual(spamNFTs.count, 1)
        XCTAssertEqual(spamNFTs[safe: 0]?.token.symbol, self.spamEthNFT.symbol)
        XCTAssertEqual(spamNFTs[safe: 0]?.nftMetadata?.imageURLString, self.mockERC721Metadata.imageURLString)
        XCTAssertEqual(spamNFTs[safe: 0]?.nftMetadata?.name, self.mockERC721Metadata.name)
      }.store(in: &cancellables)
    
    store.update()
    await fulfillment(of: [userSpamNFTsException], timeout: 1)
    cancellables.removeAll()
  }
}
