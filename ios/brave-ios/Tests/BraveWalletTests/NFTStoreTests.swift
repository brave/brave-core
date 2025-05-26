// Copyright 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveCore
import Combine
import Preferences
import XCTest

@testable import BraveWallet

class NFTStoreTests: XCTestCase {

  private var cancellables: Set<AnyCancellable> = .init()
  private var isLocked = true

  let ethNetwork: BraveWallet.NetworkInfo = .mockMainnet
  let ethAccount1: BraveWallet.AccountInfo = .mockEthAccount
  let ethAccount2 = (BraveWallet.AccountInfo.mockEthAccount.copy() as! BraveWallet.AccountInfo).then
  {
    $0.address = "mock_eth_id_2"
    $0.accountId.uniqueKey = $0.address
    $0.name = "Ethereum Account 2"
  }
  let unownedEthNFT =
    (BraveWallet.BlockchainToken.mockERC721NFTToken.copy() as! BraveWallet.BlockchainToken).then {
      $0.contractAddress = "0xbbbbbbbbbb222222222233333333334444444444"
      $0.name = "Unowned NFT"
    }
  let invisibleEthNFT =
    (BraveWallet.BlockchainToken.mockERC721NFTToken.copy() as! BraveWallet.BlockchainToken).then {
      $0.contractAddress = "0xbbbbbbbbbb222222222233333333335555555555"
      $0.name = "Invisible NFT"
      $0.visible = false
    }

  let solNetwork: BraveWallet.NetworkInfo = .mockSolana
  let solAccount: BraveWallet.AccountInfo = .mockSolAccount

  let mockERC721Metadata: BraveWallet.NftMetadata = .init(
    name: "mock nft name",
    description: "mock nft description",
    image: "mock.image.url",
    imageData: "mock.image.data",
    externalUrl: "mock.external.url",
    attributes: [],
    backgroundColor: "mock.backgroundColor",
    animationUrl: "mock.animation.url",
    youtubeUrl: "mock.youtube.url",
    collection: "mock.collection"
  )
  let mockInvisibleERC721Metadata: BraveWallet.NftMetadata = .init(
    name: "mock invisible nft name",
    description: "mock invisible nft description",
    image: "mock.invisible.image.url",
    imageData: "mock.invisible.image.data",
    externalUrl: "mock.invisible.external.url",
    attributes: [],
    backgroundColor: "invisible.mock.backgroundColor",
    animationUrl: "mock.invisible.animation.url",
    youtubeUrl: "mock.invisible.youtube.url",
    collection: "mock.invisible.collection"
  )
  let mockUnownedERC721Metadata: BraveWallet.NftMetadata = .init(
    name: "mock unowned nft name",
    description: "mock unowned nft description",
    image: "mock.unowned.image.url",
    imageData: "mock.unowned.image.data",
    externalUrl: "mock.unowned.external.url",
    attributes: [],
    backgroundColor: "mock.unowned.backgroundColor",
    animationUrl: "mock.unowned.animation.url",
    youtubeUrl: "mock.unowned.youtube.url",
    collection: "mock.unowned.collection"
  )
  let mockSpamMetadata: BraveWallet.NftMetadata = .init(
    name: "mock spam nft name",
    description: "mock spam nft description",
    image: "mock.spam.image.url",
    imageData: "mock.spam.image.data",
    externalUrl: "mock.spam.external.url",
    attributes: [],
    backgroundColor: "mock.spam.backgroundColor",
    animationUrl: "mock.spam.animation.url",
    youtubeUrl: "mock.spam.youtube.url",
    collection: "mock.spam.collection"
  )
  let mockSolMetadata: BraveWallet.NftMetadata = .init(
    name: "sol mock nft name",
    description: "sol mock nft description",
    image: "sol.mock.image.url",
    imageData: "sol.mock.image.data",
    externalUrl: "sol.mock.external.url",
    attributes: [],
    backgroundColor: "sol.mock.backgroundColor",
    animationUrl: "sol.mock.animation.url",
    youtubeUrl: "sol.mock.youtube.url",
    collection: "sol.mock.collection"
  )

  let spamEthNFT =
    (BraveWallet.BlockchainToken.mockERC721NFTToken.copy() as! BraveWallet.BlockchainToken).then {
      $0.contractAddress = "0xbbbbbbbbbb222222222233333333336666666666"
      $0.name = "Spam Eth NFT"
      $0.visible = false
      $0.isSpam = true
    }

  private func setupServices(
    mockEthUserAssets: [BraveWallet.BlockchainToken],
    mockSolUserAssets: [BraveWallet.BlockchainToken],
    mockNFTMetadata: [BraveWallet.BlockchainToken: BraveWallet.NftMetadata]
  ) -> (
    BraveWallet.TestKeyringService, BraveWallet.TestJsonRpcService,
    BraveWallet.TestBraveWalletService, BraveWallet.TestAssetRatioService,
    TestableWalletUserAssetManager, BraveWallet.TestTxService
  ) {
    let keyringService = BraveWallet.TestKeyringService()
    keyringService._addObserver = { _ in }
    keyringService._isLocked = { [weak self] completion in
      // unlocked would cause `update()` from call in `init` to be called prior to test being setup.g
      completion(self?.isLocked ?? true)
    }
    keyringService._allAccounts = {
      $0(
        .init(
          accounts: [self.solAccount, self.ethAccount1, self.ethAccount2],
          selectedAccount: self.ethAccount1,
          ethDappSelectedAccount: self.ethAccount1,
          solDappSelectedAccount: self.solAccount
        )
      )
    }

    let rpcService = MockJsonRpcService()
    rpcService._nftBalances = { accountAddress, nftIdentifiers, completion in
      var balances: [NSNumber] = []
      for nft in nftIdentifiers {
        if nft.contractAddress.caseInsensitiveCompare(
          BraveWallet.BlockchainToken.mockERC721NFTToken.contractAddress
        ) == .orderedSame,
          accountAddress.caseInsensitiveCompare(
            self.ethAccount1.address
          ) == .orderedSame
        {
          balances.append(1)
        } else if nft.contractAddress.caseInsensitiveCompare(
          self.invisibleEthNFT.contractAddress
        ) == .orderedSame,
          accountAddress.caseInsensitiveCompare(
            self.ethAccount1.address
          ) == .orderedSame
        {
          balances.append(1)
        } else if nft.contractAddress.caseInsensitiveCompare(
          mockSolUserAssets[safe: 2]?.contractAddress ?? ""
        ) == .orderedSame {
          balances.append(1)
        }
      }
      completion(balances, "")
    }
    rpcService._nftMetadatas = { nftIdentifiers, completion in
      var allMetadata = [BraveWallet.NftMetadata]()
      for identifier in nftIdentifiers {
        for (key, value) in mockNFTMetadata {
          if identifier.chainId.coin == key.coin,
            identifier.chainId.chainId == key.chainId,
            identifier.contractAddress == key.contractAddress,
            identifier.tokenId == key.tokenId
          {
            allMetadata.append(value)
          }
        }
      }
      completion(allMetadata, "")
    }
    rpcService._splTokenAccountBalance = { accountAddress, tokenMintAddress, chainId, completion in
      completion("", 0, "", .internalError, "Error")
    }
    let walletService = BraveWallet.TestBraveWalletService()
    walletService._addObserver = { _ in }
    walletService._simpleHashSpamNfTs = { walletAddress, chainIds, _, completion in
      if walletAddress == self.ethAccount1.address,
        chainIds.contains(BraveWallet.ChainId(coin: .eth, chainId: BraveWallet.MainnetChainId))
      {
        completion([self.spamEthNFT], nil)
      } else {
        completion([], nil)
      }
    }
    let assetRatioService = BraveWallet.TestAssetRatioService()

    let mockAssetManager = TestableWalletUserAssetManager()
    mockAssetManager._getUserAssets = { networks, visible in
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
        ),
      ].filter { networkAsset in networks.contains(where: { $0 == networkAsset.network }) }
    }
    mockAssetManager._getAllUserNFTs = { networks, isSpam in
      [
        NetworkAssets(
          network: .mockSolana,
          tokens: mockSolUserAssets.filter({ $0.isSpam == isSpam }),
          sortOrder: 0
        ),
        NetworkAssets(
          network: .mockMainnet,
          tokens: mockEthUserAssets.filter({ $0.isSpam == isSpam }),
          sortOrder: 1
        ),
      ].filter { networkAsset in networks.contains(where: { $0 == networkAsset.network }) }
    }

    let txService = BraveWallet.TestTxService()
    txService._addObserver = { _ in
    }

    return (
      keyringService, rpcService, walletService, assetRatioService, mockAssetManager, txService
    )
  }

  override func setUp() {
    resetFilters()
    isLocked = true
  }
  override func tearDown() {
    resetFilters()
    isLocked = true
  }
  private func resetFilters() {
    Preferences.Wallet.groupByFilter.reset()
    Preferences.Wallet.isHidingUnownedNFTsFilter.reset()
    Preferences.Wallet.isShowingNFTNetworkLogoFilter.reset()
    Preferences.Wallet.nonSelectedAccountsFilter.reset()
    Preferences.Wallet.nonSelectedNetworksFilter.reset()
  }

  // MARK: Group By `None`
  func testUpdate() async {
    let mockEthUserAssets: [BraveWallet.BlockchainToken] = [
      .previewToken.copy(asVisibleAsset: true),
      .mockUSDCToken.copy(asVisibleAsset: false),  // Verify non-visible assets not displayed #6386
      .mockERC721NFTToken,
      unownedEthNFT,
      invisibleEthNFT,
    ]
    let mockSolUserAssets: [BraveWallet.BlockchainToken] = [
      BraveWallet.NetworkInfo.mockSolana.nativeToken.copy(asVisibleAsset: true),
      .mockSpdToken.copy(asVisibleAsset: false),  // Verify non-visible assets not displayed #6386
      .mockSolanaNFTToken,
    ]

    // setup test services
    let (
      keyringService, rpcService, walletService, assetRatioService, mockAssetManager, txService
    ) = setupServices(
      mockEthUserAssets: mockEthUserAssets,
      mockSolUserAssets: mockSolUserAssets,
      mockNFTMetadata: [
        .mockERC721NFTToken: mockERC721Metadata,
        unownedEthNFT: mockUnownedERC721Metadata,
        invisibleEthNFT: mockInvisibleERC721Metadata,
        .mockSolanaNFTToken: mockSolMetadata,
      ]
    )

    // setup store
    let store = NFTStore(
      keyringService: keyringService,
      rpcService: rpcService,
      walletService: walletService,
      assetRatioService: assetRatioService,
      blockchainRegistry: BraveWallet.TestBlockchainRegistry(),
      ipfsApi: TestIpfsAPI(),
      walletP3A: TestBraveWalletP3A(),
      userAssetManager: mockAssetManager,
      txService: txService
    )
    // test that `update()` will assign new value to `userNFTs` publisher
    let userVisibleNFTsException = expectation(description: "update-userVisibleNFTs")
    XCTAssertTrue(store.userNFTGroups.isEmpty)  // Initial state
    store.$userNFTGroups
      .dropFirst()
      .collect(3)
      .sink { userNFTGroups in
        defer { userVisibleNFTsException.fulfill() }
        // empty nfts, populated w/ balance nfts, populated w/ metadata
        XCTAssertEqual(userNFTGroups.count, 3)
        guard let lastUpdatedUserNFTGroups = userNFTGroups.last else {
          XCTFail("Unexpected test result")
          return
        }
        XCTAssertEqual(lastUpdatedUserNFTGroups.count, 1)
        guard let visibleNFTs = lastUpdatedUserNFTGroups.first?.assets.filter(\.token.visible)
        else {
          XCTFail("Unexpected test result")
          return
        }

        XCTAssertEqual(visibleNFTs.count, 3)  // Solana NFT, Erc721, unowned Erc721
        XCTAssertEqual(visibleNFTs[safe: 0]?.token.symbol, mockSolUserAssets[safe: 2]?.symbol)
        XCTAssertEqual(
          visibleNFTs[safe: 0]?.nftMetadata?.image,
          self.mockSolMetadata.image
        )
        XCTAssertEqual(visibleNFTs[safe: 0]?.nftMetadata?.name, self.mockSolMetadata.name)
        XCTAssertEqual(
          visibleNFTs[safe: 0]?.nftMetadata?.desc,
          self.mockSolMetadata.desc
        )
        XCTAssertEqual(visibleNFTs[safe: 1]?.token.symbol, mockEthUserAssets[safe: 2]?.symbol)
        XCTAssertEqual(
          visibleNFTs[safe: 1]?.nftMetadata?.image,
          self.mockERC721Metadata.image
        )
        XCTAssertEqual(visibleNFTs[safe: 1]?.nftMetadata?.name, self.mockERC721Metadata.name)
        XCTAssertEqual(
          visibleNFTs[safe: 1]?.nftMetadata?.desc,
          self.mockERC721Metadata.desc
        )
        XCTAssertEqual(visibleNFTs[safe: 2]?.token.symbol, mockEthUserAssets[safe: 3]?.symbol)
        XCTAssertEqual(
          visibleNFTs[safe: 2]?.nftMetadata?.image,
          self.mockUnownedERC721Metadata.image
        )
        XCTAssertEqual(visibleNFTs[safe: 2]?.nftMetadata?.name, self.mockUnownedERC721Metadata.name)
        XCTAssertEqual(
          visibleNFTs[safe: 2]?.nftMetadata?.desc,
          self.mockUnownedERC721Metadata.desc
        )
      }.store(in: &cancellables)

    isLocked = false
    store.update()
    await fulfillment(of: [userVisibleNFTsException], timeout: 1)
    cancellables.removeAll()

    let defaultFilters = store.filters

    // MARK: Network Filter Test
    let networksExpectation = expectation(description: "update-networks")
    store.$userNFTGroups
      .dropFirst()
      .collect(2)
      .sink { userNFTGroups in
        defer { networksExpectation.fulfill() }
        XCTAssertEqual(userNFTGroups.count, 2)  // empty nfts, populated nfts
        guard let lastUpdatedUserNFTGroups = userNFTGroups.last else {
          XCTFail("Unexpected test result")
          return
        }
        XCTAssertEqual(lastUpdatedUserNFTGroups.count, 1)
        guard let visibleNFTs = lastUpdatedUserNFTGroups.first?.assets.filter(\.token.visible)
        else {
          XCTFail("Unexpected test result")
          return
        }

        XCTAssertEqual(visibleNFTs.count, 2)
        XCTAssertEqual(visibleNFTs[safe: 0]?.token.symbol, mockEthUserAssets[safe: 2]?.symbol)
        XCTAssertEqual(visibleNFTs[safe: 1]?.token.symbol, mockEthUserAssets[safe: 3]?.symbol)
        // solana NFT hidden
      }.store(in: &cancellables)
    store.saveFilters(
      .init(
        groupBy: defaultFilters.groupBy,
        sortOrder: defaultFilters.sortOrder,
        isHidingSmallBalances: defaultFilters.isHidingSmallBalances,
        isHidingUnownedNFTs: defaultFilters.isHidingUnownedNFTs,
        isShowingNFTNetworkLogo: defaultFilters.isShowingNFTNetworkLogo,
        accounts: defaultFilters.accounts,
        networks: defaultFilters.networks.map {
          .init(isSelected: $0.model.coin == .eth, model: $0.model)
        }
      )
    )
    await fulfillment(of: [networksExpectation], timeout: 1)
    cancellables.removeAll()

    // MARK: Hiding Unowned Filter Test
    let hidingUnownedExpectation = expectation(description: "update-hidingUnowned")
    store.$userNFTGroups
      .dropFirst()
      .collect(3)
      .sink { userNFTGroups in
        defer { hidingUnownedExpectation.fulfill() }
        // empty nfts, populated w/ balance nfts, populated w/ metadata
        XCTAssertEqual(userNFTGroups.count, 3)
        guard let lastUpdatedUserNFTGroups = userNFTGroups.last else {
          XCTFail("Unexpected test result")
          return
        }
        XCTAssertEqual(lastUpdatedUserNFTGroups.count, 1)
        guard let visibleNFTs = lastUpdatedUserNFTGroups.first?.assets.filter(\.token.visible)
        else {
          XCTFail("Unexpected test result")
          return
        }
        XCTAssertEqual(visibleNFTs.count, 2)
        XCTAssertEqual(visibleNFTs[safe: 0]?.token.symbol, mockSolUserAssets[safe: 2]?.symbol)
        XCTAssertEqual(visibleNFTs[safe: 1]?.token.symbol, mockEthUserAssets[safe: 2]?.symbol)
      }.store(in: &cancellables)
    store.saveFilters(
      .init(
        groupBy: defaultFilters.groupBy,
        sortOrder: defaultFilters.sortOrder,
        isHidingSmallBalances: defaultFilters.isHidingSmallBalances,
        isHidingUnownedNFTs: true,
        isShowingNFTNetworkLogo: defaultFilters.isShowingNFTNetworkLogo,
        accounts: defaultFilters.accounts,
        networks: defaultFilters.networks
      )
    )
    await fulfillment(of: [hidingUnownedExpectation], timeout: 1)
    cancellables.removeAll()

    // MARK: Accounts Filter Test
    let accountsExpectation = expectation(description: "update-accounts")
    store.$userNFTGroups
      .dropFirst()
      .collect(2)
      .sink { userNFTGroups in
        defer { accountsExpectation.fulfill() }
        XCTAssertEqual(userNFTGroups.count, 2)  // empty nfts, populated nfts
        guard let lastUpdatedNFTGroups = userNFTGroups.last else {
          XCTFail("Unexpected test result")
          return
        }
        XCTAssertEqual(lastUpdatedNFTGroups.count, 1)
        guard let visibleNFTs = lastUpdatedNFTGroups.first?.assets.filter(\.token.visible) else {
          XCTFail("Unexpected test result")
          return
        }
        XCTAssertEqual(visibleNFTs.count, 1)
        XCTAssertEqual(visibleNFTs[safe: 0]?.token.symbol, mockEthUserAssets[safe: 2]?.symbol)
      }.store(in: &cancellables)
    store.saveFilters(
      .init(
        groupBy: defaultFilters.groupBy,
        sortOrder: defaultFilters.sortOrder,
        isHidingSmallBalances: defaultFilters.isHidingSmallBalances,
        isHidingUnownedNFTs: true,
        isShowingNFTNetworkLogo: defaultFilters.isShowingNFTNetworkLogo,
        accounts: defaultFilters.accounts.map {  // only ethereum accounts selected
          .init(isSelected: $0.model.coin == .eth, model: $0.model)
        },
        networks: defaultFilters.networks
      )
    )
    await fulfillment(of: [accountsExpectation], timeout: 1)
    cancellables.removeAll()
  }

  // MARK: Group By `None`
  func testUpdateForInvisibleGroup() async {
    let mockEthUserAssets: [BraveWallet.BlockchainToken] = [
      .previewToken.copy(asVisibleAsset: true),
      .mockUSDCToken.copy(asVisibleAsset: false),  // Verify non-visible assets not displayed #6386
      .mockERC721NFTToken,
      unownedEthNFT,
      invisibleEthNFT,
    ]
    let mockSolUserAssets: [BraveWallet.BlockchainToken] = [
      BraveWallet.NetworkInfo.mockSolana.nativeToken.copy(asVisibleAsset: true),
      .mockSpdToken.copy(asVisibleAsset: false),  // Verify non-visible assets not displayed #6386
      .mockSolanaNFTToken,
    ]

    // setup test services
    let (
      keyringService, rpcService, walletService, assetRatioService, mockAssetManager, txService
    ) = setupServices(
      mockEthUserAssets: mockEthUserAssets,
      mockSolUserAssets: mockSolUserAssets,
      mockNFTMetadata: [
        .mockERC721NFTToken: mockERC721Metadata,
        unownedEthNFT: mockUnownedERC721Metadata,
        invisibleEthNFT: mockInvisibleERC721Metadata,
        .mockSolanaNFTToken: mockSolMetadata,
      ]
    )

    // setup store
    let store = NFTStore(
      keyringService: keyringService,
      rpcService: rpcService,
      walletService: walletService,
      assetRatioService: assetRatioService,
      blockchainRegistry: BraveWallet.TestBlockchainRegistry(),
      ipfsApi: TestIpfsAPI(),
      walletP3A: TestBraveWalletP3A(),
      userAssetManager: mockAssetManager,
      txService: txService
    )

    // MARK: Group By: None
    // test that `update()` will assign new value to `userInvisibleNFTs` publisher
    let userHiddenNFTsException = expectation(description: "update-userInvisibleNFTs")
    store.$userNFTGroups
      .dropFirst()
      .collect(3)
      .sink { userNFTGroups in
        defer { userHiddenNFTsException.fulfill() }
        // empty nfts, populated w/ balance nfts, populated w/ metadata
        XCTAssertEqual(userNFTGroups.count, 3)
        guard let lastUpdatedNFTGroups = userNFTGroups.last else {
          XCTFail("Unexpected test result")
          return
        }
        XCTAssertEqual(lastUpdatedNFTGroups.count, 1)  // invisible Erc721
        guard let onlyGroupAssets = lastUpdatedNFTGroups.first?.assets else {
          XCTFail("Unexpected test result")
          return
        }
        let hiddenNFTs = onlyGroupAssets.filter { !$0.token.visible }

        XCTAssertEqual(hiddenNFTs.count, 1)
        XCTAssertEqual(hiddenNFTs[safe: 0]?.token.symbol, mockEthUserAssets[safe: 4]?.symbol)
        XCTAssertEqual(
          hiddenNFTs[safe: 0]?.nftMetadata?.image,
          self.mockInvisibleERC721Metadata.image
        )
        XCTAssertEqual(
          hiddenNFTs[safe: 0]?.nftMetadata?.name,
          self.mockInvisibleERC721Metadata.name
        )
        XCTAssertEqual(
          hiddenNFTs[safe: 0]?.nftMetadata?.desc,
          self.mockInvisibleERC721Metadata.desc
        )
      }.store(in: &cancellables)
    isLocked = false
    store.update()
    await fulfillment(of: [userHiddenNFTsException], timeout: 1)
    cancellables.removeAll()
  }

  // MARK: Group By `None`
  func testSpamOnlyFromSimpleHash() async {
    let mockEthUserAssets: [BraveWallet.BlockchainToken] = [
      .previewToken.copy(asVisibleAsset: true),
      .mockUSDCToken.copy(asVisibleAsset: false),  // Verify non-visible assets not displayed #6386
      .mockERC721NFTToken,
      unownedEthNFT,
      invisibleEthNFT,
    ]
    let mockSolUserAssets: [BraveWallet.BlockchainToken] = [
      BraveWallet.NetworkInfo.mockSolana.nativeToken.copy(asVisibleAsset: true),
      .mockSpdToken.copy(asVisibleAsset: false),  // Verify non-visible assets not displayed #6386
      .mockSolanaNFTToken,
    ]

    // setup test services
    let (
      keyringService, rpcService, walletService, assetRatioService, mockAssetManager, txService
    ) = setupServices(
      mockEthUserAssets: mockEthUserAssets,
      mockSolUserAssets: mockSolUserAssets,
      mockNFTMetadata: [
        .mockERC721NFTToken: mockERC721Metadata,
        unownedEthNFT: mockUnownedERC721Metadata,
        invisibleEthNFT: mockERC721Metadata,
        spamEthNFT: mockSpamMetadata,
        .mockSolanaNFTToken: mockSolMetadata,
      ]
    )

    // MARK: Group By: None
    // setup store
    let store = NFTStore(
      keyringService: keyringService,
      rpcService: rpcService,
      walletService: walletService,
      assetRatioService: assetRatioService,
      blockchainRegistry: BraveWallet.TestBlockchainRegistry(),
      ipfsApi: TestIpfsAPI(),
      walletP3A: TestBraveWalletP3A(),
      userAssetManager: mockAssetManager,
      txService: txService
    )

    // test that `update()` will assign new value to `userNFTs` publisher
    let userSpamNFTsException = expectation(description: "update-userInvisibleNFTs1")
    store.$userNFTGroups
      .dropFirst()
      .collect(3)
      .sink { userNFTGroups in
        defer { userSpamNFTsException.fulfill() }
        // empty nfts, populated w/ balance nfts, populated w/ metadata
        XCTAssertEqual(userNFTGroups.count, 3)
        guard let lastUpdatedUserNFTGroups = userNFTGroups.last else {
          XCTFail("Unexpected test result")
          return
        }
        XCTAssertEqual(lastUpdatedUserNFTGroups.count, 1)
        guard let spamNFTs = lastUpdatedUserNFTGroups.first?.assets.filter(\.token.isSpam) else {
          XCTFail("Unexpected test result")
          return
        }
        XCTAssertEqual(spamNFTs.count, 1)
        XCTAssertEqual(spamNFTs[safe: 0]?.token.symbol, self.spamEthNFT.symbol)
        XCTAssertEqual(
          spamNFTs[safe: 0]?.nftMetadata?.image,
          self.mockSpamMetadata.image
        )
        XCTAssertEqual(spamNFTs[safe: 0]?.nftMetadata?.name, self.mockSpamMetadata.name)
      }.store(in: &cancellables)

    isLocked = false
    store.fetchJunkNFTs()
    await fulfillment(of: [userSpamNFTsException], timeout: 1)
    cancellables.removeAll()
  }

  // MARK: Group By `None`
  func testSpamFromSimpleHashAndUserMarked() async {
    let mockEthUserAssets: [BraveWallet.BlockchainToken] = [
      .previewToken.copy(asVisibleAsset: true),
      .mockUSDCToken.copy(asVisibleAsset: false),  // Verify non-visible assets not displayed #6386
      .mockERC721NFTToken,
      invisibleEthNFT,
    ]
    let mockSolUserAssets: [BraveWallet.BlockchainToken] = [
      BraveWallet.NetworkInfo.mockSolana.nativeToken.copy(asVisibleAsset: true),
      .mockSpdToken.copy(asVisibleAsset: false),  // Verify non-visible assets not displayed #6386
      .mockSolanaNFTToken.copy(asVisibleAsset: false, isSpam: true),
    ]

    // setup test services
    let (
      keyringService, rpcService, walletService, assetRatioService, mockAssetManager, txService
    ) = setupServices(
      mockEthUserAssets: mockEthUserAssets,
      mockSolUserAssets: mockSolUserAssets,
      mockNFTMetadata: [
        .mockERC721NFTToken: mockERC721Metadata,
        invisibleEthNFT: mockERC721Metadata,
        spamEthNFT: mockSpamMetadata,
        .mockSolanaNFTToken: mockSolMetadata,
      ]
    )

    // setup store
    let store = NFTStore(
      keyringService: keyringService,
      rpcService: rpcService,
      walletService: walletService,
      assetRatioService: assetRatioService,
      blockchainRegistry: BraveWallet.TestBlockchainRegistry(),
      ipfsApi: TestIpfsAPI(),
      walletP3A: TestBraveWalletP3A(),
      userAssetManager: mockAssetManager,
      txService: txService
    )

    // test that `update()` will assign new value to `userNFTs` publisher
    let userSpamNFTsException = expectation(description: "update-userSpamNFTsException2")
    store.$userNFTGroups
      .dropFirst()
      .collect(3)
      .sink { userNFTGroups in
        defer { userSpamNFTsException.fulfill() }
        // empty nfts, populated w/ balance nfts, populated w/ metadata
        XCTAssertEqual(userNFTGroups.count, 3)
        guard let lastUpdatedUserNFTGroups = userNFTGroups.last else {
          XCTFail("Unexpected test result")
          return
        }
        XCTAssertEqual(lastUpdatedUserNFTGroups.count, 1)
        guard let spamNFTs = lastUpdatedUserNFTGroups.first?.assets.filter(\.token.isSpam) else {
          XCTFail("Unexpected test result")
          return
        }
        XCTAssertEqual(spamNFTs.count, 2)
        XCTAssertEqual(
          spamNFTs[safe: 0]?.token.symbol,
          BraveWallet.BlockchainToken.mockSolanaNFTToken.symbol
        )
        XCTAssertEqual(
          spamNFTs[safe: 0]?.nftMetadata?.image,
          self.mockSolMetadata.image
        )
        XCTAssertEqual(spamNFTs[safe: 0]?.nftMetadata?.name, self.mockSolMetadata.name)
        XCTAssertEqual(spamNFTs[safe: 1]?.token.symbol, self.spamEthNFT.symbol)
        XCTAssertEqual(
          spamNFTs[safe: 1]?.nftMetadata?.image,
          self.mockSpamMetadata.image
        )
        XCTAssertEqual(spamNFTs[safe: 1]?.nftMetadata?.name, self.mockSpamMetadata.name)
      }.store(in: &cancellables)

    isLocked = false
    store.fetchJunkNFTs()
    await fulfillment(of: [userSpamNFTsException], timeout: 1)
    cancellables.removeAll()
  }

  // MARK: Group By `None`
  func testSpamDuplicationFromSimpleHashAndUserMarked() async {
    let mockEthUserAssets: [BraveWallet.BlockchainToken] = [
      .previewToken.copy(asVisibleAsset: true),
      .mockUSDCToken.copy(asVisibleAsset: false),  // Verify non-visible assets not displayed #6386
      .mockERC721NFTToken,
      invisibleEthNFT,
      spamEthNFT,
    ]
    let mockSolUserAssets: [BraveWallet.BlockchainToken] = [
      BraveWallet.NetworkInfo.mockSolana.nativeToken.copy(asVisibleAsset: true),
      .mockSpdToken.copy(asVisibleAsset: false),  // Verify non-visible assets not displayed #6386
      .mockSolanaNFTToken,
    ]

    // setup test services
    let (
      keyringService, rpcService, walletService, assetRatioService, mockAssetManager, txService
    ) = setupServices(
      mockEthUserAssets: mockEthUserAssets,
      mockSolUserAssets: mockSolUserAssets,
      mockNFTMetadata: [
        .mockERC721NFTToken: mockERC721Metadata,
        invisibleEthNFT: mockERC721Metadata,
        spamEthNFT: mockSpamMetadata,
        .mockSolanaNFTToken: mockSolMetadata,
      ]
    )

    // setup store
    let store = NFTStore(
      keyringService: keyringService,
      rpcService: rpcService,
      walletService: walletService,
      assetRatioService: assetRatioService,
      blockchainRegistry: BraveWallet.TestBlockchainRegistry(),
      ipfsApi: TestIpfsAPI(),
      walletP3A: TestBraveWalletP3A(),
      userAssetManager: mockAssetManager,
      txService: txService
    )

    // test that `update()` will assign new value to `userSpamNFTs` publisher
    let userSpamNFTsException = expectation(description: "update-userSpamNFTsException2")
    store.$userNFTGroups
      .dropFirst()
      .collect(3)
      .sink { userNFTGroups in
        defer { userSpamNFTsException.fulfill() }
        // empty nfts, populated w/ balance nfts, populated w/ metadata
        XCTAssertEqual(userNFTGroups.count, 3)
        guard let lastUpdatedUserNFTGroups = userNFTGroups.last else {
          XCTFail("Unexpected test result")
          return
        }
        XCTAssertEqual(lastUpdatedUserNFTGroups.count, 1)
        guard let spamNFTs = lastUpdatedUserNFTGroups.first?.assets.filter(\.token.isSpam) else {
          XCTFail("Unexpected test result")
          return
        }
        XCTAssertEqual(spamNFTs.count, 1)
        XCTAssertEqual(spamNFTs[safe: 0]?.token.symbol, self.spamEthNFT.symbol)
        XCTAssertEqual(
          spamNFTs[safe: 0]?.nftMetadata?.image,
          self.mockSpamMetadata.image
        )
        XCTAssertEqual(spamNFTs[safe: 0]?.nftMetadata?.name, self.mockSpamMetadata.name)
        XCTAssertEqual(spamNFTs[safe: 0]?.nftMetadata?.desc, self.mockSpamMetadata.desc)
      }.store(in: &cancellables)

    isLocked = false
    store.fetchJunkNFTs()
    await fulfillment(of: [userSpamNFTsException], timeout: 1)
    cancellables.removeAll()
  }

  // MARK: Group by `Accounts` with `displayType`: `visible`
  func testUpdateGroupByAccountsVisibleNFTs() async {
    let mockEthUserAssets: [BraveWallet.BlockchainToken] = [
      .previewToken.copy(asVisibleAsset: true),
      .mockUSDCToken.copy(asVisibleAsset: false),
      .mockERC721NFTToken,
      unownedEthNFT,
      invisibleEthNFT,
    ]
    let mockSolUserAssets: [BraveWallet.BlockchainToken] = [
      BraveWallet.NetworkInfo.mockSolana.nativeToken.copy(asVisibleAsset: true),
      .mockSpdToken.copy(asVisibleAsset: false),
      .mockSolanaNFTToken,
    ]

    // setup test services
    let (
      keyringService, rpcService, walletService, assetRatioService, mockAssetManager, txService
    ) = setupServices(
      mockEthUserAssets: mockEthUserAssets,
      mockSolUserAssets: mockSolUserAssets,
      mockNFTMetadata: [
        .mockERC721NFTToken: mockERC721Metadata,
        unownedEthNFT: mockUnownedERC721Metadata,
        invisibleEthNFT: mockERC721Metadata,
        .mockSolanaNFTToken: mockSolMetadata,
      ]
    )

    // setup store
    let store = NFTStore(
      keyringService: keyringService,
      rpcService: rpcService,
      walletService: walletService,
      assetRatioService: assetRatioService,
      blockchainRegistry: BraveWallet.TestBlockchainRegistry(),
      ipfsApi: TestIpfsAPI(),
      walletP3A: TestBraveWalletP3A(),
      userAssetManager: mockAssetManager,
      txService: txService
    )

    let defaultFilters = store.filters

    let groupByAccountVisibleExpectation = expectation(
      description: "groupByAccountVisibleExpectation"
    )
    store.$userNFTGroups
      .dropFirst()
      .collect(3)
      .sink { userNFTGroups in
        defer { groupByAccountVisibleExpectation.fulfill() }
        // empty nfts, populated w/ balance nfts, populated w/ metadata
        XCTAssertEqual(userNFTGroups.count, 3)
        guard let lastUpdatedUserNFTGroups = userNFTGroups.last else {
          XCTFail("Unexpected test result")
          return
        }
        XCTAssertEqual(lastUpdatedUserNFTGroups.count, 3)
        guard
          let groupOneVisibleNFTs = lastUpdatedUserNFTGroups.first?.assets.filter(\.token.visible),
          let groupTwoVisibleNFTs = lastUpdatedUserNFTGroups[safe: 1]?.assets.filter(
            \.token.visible
          ),
          let groupThreeVisibleNFTs = lastUpdatedUserNFTGroups[safe: 2]?.assets.filter(
            \.token.visible
          )
        else {
          XCTFail("Unexpected test result")
          return
        }
        XCTAssertEqual(groupOneVisibleNFTs.count, 1)
        XCTAssertEqual(
          groupOneVisibleNFTs[safe: 0]?.token.symbol,
          mockSolUserAssets[safe: 2]?.symbol
        )
        XCTAssertEqual(groupTwoVisibleNFTs.count, 1)
        XCTAssertEqual(
          groupTwoVisibleNFTs[safe: 0]?.token.symbol,
          mockEthUserAssets[safe: 2]?.symbol
        )
        XCTAssertTrue(groupThreeVisibleNFTs.isEmpty)
      }.store(in: &cancellables)
    isLocked = false
    store.saveFilters(
      .init(
        groupBy: .accounts,
        sortOrder: defaultFilters.sortOrder,
        isHidingSmallBalances: defaultFilters.isHidingSmallBalances,
        isHidingUnownedNFTs: true,
        isShowingNFTNetworkLogo: defaultFilters.isShowingNFTNetworkLogo,
        accounts: defaultFilters.accounts,
        networks: defaultFilters.networks
      )
    )
    await fulfillment(of: [groupByAccountVisibleExpectation], timeout: 1)
    cancellables.removeAll()
  }

  // MARK: Group by `Accounts` with `displayType`: `hidden`
  func testUpdateGroupByAccountsHiddenNFTs() async {
    let mockEthUserAssets: [BraveWallet.BlockchainToken] = [
      .previewToken.copy(asVisibleAsset: true),
      .mockUSDCToken.copy(asVisibleAsset: false),
      .mockERC721NFTToken.copy(asVisibleAsset: false),
      unownedEthNFT,
      invisibleEthNFT,
    ]
    let mockSolUserAssets: [BraveWallet.BlockchainToken] = [
      BraveWallet.NetworkInfo.mockSolana.nativeToken.copy(asVisibleAsset: true),
      .mockSpdToken.copy(asVisibleAsset: false),
      .mockSolanaNFTToken.copy(asVisibleAsset: false),
    ]

    // setup test services
    let (
      keyringService, rpcService, walletService, assetRatioService, mockAssetManager, txService
    ) = setupServices(
      mockEthUserAssets: mockEthUserAssets,
      mockSolUserAssets: mockSolUserAssets,
      mockNFTMetadata: [
        .mockERC721NFTToken: mockERC721Metadata,
        unownedEthNFT: mockUnownedERC721Metadata,
        invisibleEthNFT: mockInvisibleERC721Metadata,
        .mockSolanaNFTToken: mockSolMetadata,
      ]
    )

    // setup store
    let store = NFTStore(
      keyringService: keyringService,
      rpcService: rpcService,
      walletService: walletService,
      assetRatioService: assetRatioService,
      blockchainRegistry: BraveWallet.TestBlockchainRegistry(),
      ipfsApi: TestIpfsAPI(),
      walletP3A: TestBraveWalletP3A(),
      userAssetManager: mockAssetManager,
      txService: txService
    )

    let defaultFilters = store.filters

    let groupByAccountHiddenNFTsException = expectation(
      description: "groupByAccountHiddenNFTsException"
    )
    store.$userNFTGroups
      .dropFirst()
      .collect(3)
      .sink { userNFTGroups in
        defer { groupByAccountHiddenNFTsException.fulfill() }
        // empty nfts, populated w/ balance nfts, populated w/ metadata
        XCTAssertEqual(userNFTGroups.count, 3)
        guard let lastUpdatedNFTGroups = userNFTGroups.last else {
          XCTFail("Unexpected test result")
          return
        }
        XCTAssertEqual(lastUpdatedNFTGroups.count, 3)
        guard
          let groupOne = lastUpdatedNFTGroups.first,
          let groupTwo = lastUpdatedNFTGroups[safe: 1],
          let groupThree = lastUpdatedNFTGroups[safe: 2]
        else {
          XCTFail("Unexpected test result")
          return
        }
        let groupOneHiddenNFTs = groupOne.assets.filter {
          !$0.token.visible || $0.token.isSpam
        }
        let groupTwoHiddenNFTs = groupTwo.assets.filter {
          !$0.token.visible || $0.token.isSpam
        }
        let groupThreeHiddenNFTs = groupThree.assets.filter {
          !$0.token.visible || $0.token.isSpam
        }
        XCTAssertEqual(groupOneHiddenNFTs.count, 1)
        XCTAssertEqual(
          groupOneHiddenNFTs[safe: 0]?.token.symbol,
          mockSolUserAssets[safe: 2]?.symbol
        )
        XCTAssertEqual(
          groupOneHiddenNFTs[safe: 0]?.nftMetadata?.image,
          self.mockSolMetadata.image
        )
        XCTAssertEqual(groupOneHiddenNFTs[safe: 0]?.nftMetadata?.name, self.mockSolMetadata.name)
        XCTAssertEqual(groupTwoHiddenNFTs.count, 2)
        XCTAssertEqual(
          groupTwoHiddenNFTs[safe: 0]?.token.symbol,
          mockEthUserAssets[safe: 2]?.symbol
        )
        XCTAssertEqual(
          groupTwoHiddenNFTs[safe: 0]?.nftMetadata?.image,
          self.mockERC721Metadata.image
        )
        XCTAssertEqual(
          groupTwoHiddenNFTs[safe: 0]?.nftMetadata?.name,
          self.mockERC721Metadata.name
        )
        XCTAssertEqual(
          groupTwoHiddenNFTs[safe: 0]?.nftMetadata?.desc,
          self.mockERC721Metadata.desc
        )
        XCTAssertEqual(
          groupTwoHiddenNFTs[safe: 1]?.token.symbol,
          mockEthUserAssets[safe: 4]?.symbol
        )
        XCTAssertEqual(
          groupTwoHiddenNFTs[safe: 1]?.nftMetadata?.image,
          self.mockInvisibleERC721Metadata.image
        )
        XCTAssertEqual(
          groupTwoHiddenNFTs[safe: 1]?.nftMetadata?.name,
          self.mockInvisibleERC721Metadata.name
        )
        XCTAssertEqual(
          groupTwoHiddenNFTs[safe: 1]?.nftMetadata?.desc,
          self.mockInvisibleERC721Metadata.desc
        )
        XCTAssertEqual(groupThreeHiddenNFTs.count, 0)
      }.store(in: &cancellables)
    isLocked = false
    store.saveFilters(
      .init(
        groupBy: .accounts,
        sortOrder: defaultFilters.sortOrder,
        isHidingSmallBalances: defaultFilters.isHidingSmallBalances,
        isHidingUnownedNFTs: true,
        isShowingNFTNetworkLogo: defaultFilters.isShowingNFTNetworkLogo,
        accounts: defaultFilters.accounts,
        networks: defaultFilters.networks
      )
    )
    await fulfillment(of: [groupByAccountHiddenNFTsException], timeout: 1)
    cancellables.removeAll()
  }

  // MARK: Group by `Networks` with `displayType`: `visible`
  func testUpdateGroupByNetworksVisibleNFTs() async {
    let mockEthUserAssets: [BraveWallet.BlockchainToken] = [
      .previewToken.copy(asVisibleAsset: true),
      .mockERC721NFTToken,
      unownedEthNFT,
      invisibleEthNFT,
    ]
    let mockSolUserAssets: [BraveWallet.BlockchainToken] = [
      BraveWallet.NetworkInfo.mockSolana.nativeToken.copy(asVisibleAsset: true),
      .mockSpdToken.copy(asVisibleAsset: false),
      .mockSolanaNFTToken,
    ]

    // setup test services
    let (
      keyringService, rpcService, walletService, assetRatioService, mockAssetManager, txService
    ) = setupServices(
      mockEthUserAssets: mockEthUserAssets,
      mockSolUserAssets: mockSolUserAssets,
      mockNFTMetadata: [
        .mockERC721NFTToken: mockERC721Metadata,
        unownedEthNFT: mockUnownedERC721Metadata,
        invisibleEthNFT: mockERC721Metadata,
        .mockSolanaNFTToken: mockSolMetadata,
      ]
    )

    // setup store
    let store = NFTStore(
      keyringService: keyringService,
      rpcService: rpcService,
      walletService: walletService,
      assetRatioService: assetRatioService,
      blockchainRegistry: BraveWallet.TestBlockchainRegistry(),
      ipfsApi: TestIpfsAPI(),
      walletP3A: TestBraveWalletP3A(),
      userAssetManager: mockAssetManager,
      txService: txService
    )

    let defaultFilters = store.filters

    let groupByNetworkVisibleExpectation = expectation(
      description: "groupByNetworkVisibleExpectation"
    )
    store.$userNFTGroups
      .dropFirst()
      .collect(3)
      .sink { userNFTGroups in
        defer { groupByNetworkVisibleExpectation.fulfill() }
        // empty nfts, populated w/ balance nfts, populated w/ metadata
        XCTAssertEqual(userNFTGroups.count, 3)
        guard let lastUpdatedUserNFTGroups = userNFTGroups.last else {
          XCTFail("Unexpected test result")
          return
        }
        // Solana mainnet, Ethereum mainnet, Polygon, Filecoin mainnet, Bitcoin mainnet, Zcash mainnet
        XCTAssertEqual(lastUpdatedUserNFTGroups.count, 6)
        guard
          let solNetworkGroupVisibleNFTs = lastUpdatedUserNFTGroups[safe: 0]?.assets.filter(
            \.token.visible
          ),
          let ethNetworkGroupVisibleNFTs = lastUpdatedUserNFTGroups[safe: 1]?.assets.filter(
            \.token.visible
          )
        else {
          XCTFail("Unexpected test result")
          return
        }
        XCTAssertEqual(solNetworkGroupVisibleNFTs.count, 1)
        XCTAssertEqual(
          solNetworkGroupVisibleNFTs[safe: 0]?.token.symbol,
          mockSolUserAssets[safe: 2]?.symbol
        )
        XCTAssertEqual(ethNetworkGroupVisibleNFTs.count, 1)  // isHidingUnownedNFTs: true
        XCTAssertEqual(
          ethNetworkGroupVisibleNFTs[safe: 0]?.token.contractAddress,
          mockEthUserAssets[safe: 1]?.contractAddress
        )
      }.store(in: &cancellables)
    isLocked = false
    store.saveFilters(
      .init(
        groupBy: .networks,
        sortOrder: defaultFilters.sortOrder,
        isHidingSmallBalances: defaultFilters.isHidingSmallBalances,
        isHidingUnownedNFTs: true,
        isShowingNFTNetworkLogo: defaultFilters.isShowingNFTNetworkLogo,
        accounts: defaultFilters.accounts,
        networks: defaultFilters.networks
      )
    )
    await fulfillment(of: [groupByNetworkVisibleExpectation], timeout: 1)
    cancellables.removeAll()
  }
  // MARK: Group by `Networks` with `displayType`: `hidden`
  func testUpdateGroupByNetworksHiddenNFTs() async {
    let mockEthUserAssets: [BraveWallet.BlockchainToken] = [
      .previewToken.copy(asVisibleAsset: true),
      .mockUSDCToken.copy(asVisibleAsset: false),
      .mockERC721NFTToken.copy(asVisibleAsset: false),
      unownedEthNFT,
      invisibleEthNFT,
    ]
    let mockSolUserAssets: [BraveWallet.BlockchainToken] = [
      BraveWallet.NetworkInfo.mockSolana.nativeToken.copy(asVisibleAsset: true),
      .mockSpdToken.copy(asVisibleAsset: false),
      .mockSolanaNFTToken.copy(asVisibleAsset: false),
    ]

    // setup test services
    let (
      keyringService, rpcService, walletService, assetRatioService, mockAssetManager, txService
    ) = setupServices(
      mockEthUserAssets: mockEthUserAssets,
      mockSolUserAssets: mockSolUserAssets,
      mockNFTMetadata: [
        .mockERC721NFTToken: mockERC721Metadata,
        unownedEthNFT: mockUnownedERC721Metadata,
        invisibleEthNFT: mockInvisibleERC721Metadata,
        .mockSolanaNFTToken: mockSolMetadata,
      ]
    )

    // setup store
    let store = NFTStore(
      keyringService: keyringService,
      rpcService: rpcService,
      walletService: walletService,
      assetRatioService: assetRatioService,
      blockchainRegistry: BraveWallet.TestBlockchainRegistry(),
      ipfsApi: TestIpfsAPI(),
      walletP3A: TestBraveWalletP3A(),
      userAssetManager: mockAssetManager,
      txService: txService
    )

    let defaultFilters = store.filters

    let groupByAccountHiddenNFTsException = expectation(
      description: "groupByAccountHiddenNFTsException"
    )
    store.$userNFTGroups
      .dropFirst()
      .collect(3)
      .sink { userNFTGroups in
        defer { groupByAccountHiddenNFTsException.fulfill() }
        // empty nfts, populated w/ balance nfts, populated w/ metadata
        XCTAssertEqual(userNFTGroups.count, 3)
        guard let lastUpdatedNFTGroups = userNFTGroups.last else {
          XCTFail("Unexpected test result")
          return
        }
        // Solana mainnet, Ethereum mainnet, Polygon, Filecoin mainnet, Bitcoin mainnet, Zcash mainnet
        XCTAssertEqual(lastUpdatedNFTGroups.count, 6)
        guard
          let solNetworkGroup = lastUpdatedNFTGroups[safe: 0],
          let ethNetworkGroup = lastUpdatedNFTGroups[safe: 1]
        else {
          XCTFail("Unexpected test result")
          return
        }
        let solGroupHiddenNFTs = solNetworkGroup.assets.filter {
          !$0.token.visible || $0.token.isSpam
        }
        let ethNetworkGroupHiddenNFTs = ethNetworkGroup.assets.filter {
          !$0.token.visible || $0.token.isSpam
        }
        XCTAssertEqual(solGroupHiddenNFTs.count, 1)
        XCTAssertEqual(
          solGroupHiddenNFTs[safe: 0]?.token.symbol,
          mockSolUserAssets[safe: 2]?.symbol
        )
        XCTAssertEqual(
          solGroupHiddenNFTs[safe: 0]?.nftMetadata?.image,
          self.mockSolMetadata.image
        )
        XCTAssertEqual(solGroupHiddenNFTs[safe: 0]?.nftMetadata?.name, self.mockSolMetadata.name)
        XCTAssertEqual(ethNetworkGroupHiddenNFTs.count, 2)
        XCTAssertEqual(
          ethNetworkGroupHiddenNFTs[safe: 0]?.token.symbol,
          mockEthUserAssets[safe: 2]?.symbol
        )
        XCTAssertEqual(
          ethNetworkGroupHiddenNFTs[safe: 0]?.nftMetadata?.image,
          self.mockERC721Metadata.image
        )
        XCTAssertEqual(
          ethNetworkGroupHiddenNFTs[safe: 0]?.nftMetadata?.name,
          self.mockERC721Metadata.name
        )
        XCTAssertEqual(
          ethNetworkGroupHiddenNFTs[safe: 1]?.token.symbol,
          mockEthUserAssets[safe: 4]?.symbol
        )
        XCTAssertEqual(
          ethNetworkGroupHiddenNFTs[safe: 1]?.nftMetadata?.image,
          self.mockInvisibleERC721Metadata.image
        )
        XCTAssertEqual(
          ethNetworkGroupHiddenNFTs[safe: 1]?.nftMetadata?.name,
          self.mockInvisibleERC721Metadata.name
        )
      }.store(in: &cancellables)
    isLocked = false
    store.saveFilters(
      .init(
        groupBy: .networks,
        sortOrder: defaultFilters.sortOrder,
        isHidingSmallBalances: defaultFilters.isHidingSmallBalances,
        isHidingUnownedNFTs: true,
        isShowingNFTNetworkLogo: defaultFilters.isShowingNFTNetworkLogo,
        accounts: defaultFilters.accounts,
        networks: defaultFilters.networks
      )
    )
    await fulfillment(of: [groupByAccountHiddenNFTsException], timeout: 1)
    cancellables.removeAll()
  }
}
