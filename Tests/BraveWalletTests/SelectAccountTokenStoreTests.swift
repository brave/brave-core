// Copyright 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Combine
import XCTest
import BraveCore
@testable import BraveWallet

@MainActor class SelectAccountTokenStoreTests: XCTestCase {
  
  private var cancellables: Set<AnyCancellable> = .init()
  
  private let allUserAssets: [BraveWallet.BlockchainToken] = [
    .previewToken.copy(asVisibleAsset: true),
    .mockUSDCToken.copy(asVisibleAsset: true).then { $0.chainId = BraveWallet.GoerliChainId },
    .mockSolToken.copy(asVisibleAsset: true),
    .mockSpdToken.copy(asVisibleAsset: false), // not visible
    .mockSolanaNFTToken.copy(asVisibleAsset: true).then { $0.chainId = BraveWallet.SolanaTestnet }
  ]
  private var allUserAssetsInNetworkAssets: [NetworkAssets] {
    [
      NetworkAssets(
        network: .mockMainnet,
        tokens: [allUserAssets[0]],
        sortOrder: 0
      ),
      NetworkAssets(
        network: .mockGoerli,
        tokens: [allUserAssets[1]],
        sortOrder: 1
      ),
      NetworkAssets(
        network: .mockSolana,
        tokens: [allUserAssets[2], allUserAssets[3]],
        sortOrder: 2
      ),
      NetworkAssets(
        network: .mockSolanaTestnet,
        tokens: [allUserAssets[4]],
        sortOrder: 3
      )
    ]
  }
  
  private let allNetworks: [BraveWallet.NetworkInfo] = [
    .mockMainnet,
    .mockGoerli,
    .mockSolana,
    .mockSolanaTestnet
  ]
  
  private let mockEthAccount2: BraveWallet.AccountInfo = .init(
    accountId: .init(
      coin: .eth,
      keyringId: BraveWallet.KeyringId.default,
      kind: .derived,
      address: "mock_eth_id_2",
      uniqueKey: "mock_eth_id_2"
    ),
    address: "mock_eth_id_2",
    name: "Ethereum Account 2",
    hardware: nil
  )
  
  private let formatter = WeiFormatter(decimalFormatStyle: .decimals(precision: 18))
  private let currencyFormatter = NumberFormatter().then { $0.numberStyle = .currency }
  
  /// Test `update()` will update `accountSections` for each account, and verify
  /// `filteredAccountSections` displays non-zero balance by default.
  func testUpdate() async {
    let mockETHBalance: Double = 0.896
    let mockETHPrice: String = "3059.99" // ETH value = $2741.75104
    let mockUSDCBalance: Double = 4
    let mockUSDCPrice: String = "1" // USDC value = $4
    let mockSOLLamportBalance: UInt64 = 3876535000 // ~3.8765 SOL
    let mockSOLBalance: Double = 3.8765 // lamports rounded
    let mockSOLPrice: String = "200" // SOL value = $775.30
    let mockNFTBalance: Double = 1
    let mockNFTMetadata: NFTMetadata = .init(
      imageURLString: "sol.mock.image.url",
      name: "sol mock nft name",
      description: "sol mock nft description"
    )
    
    let ethBalanceWei = formatter.weiString(
      from: mockETHBalance,
      radix: .hex,
      decimals: Int(allUserAssets[0].decimals)
    ) ?? ""
    let mockETHAssetPrice: BraveWallet.AssetPrice = .init(
      fromAsset: "eth", toAsset: "usd",
      price: mockETHPrice, assetTimeframeChange: "-57.23")
    let usdcBalanceWei = formatter.weiString(
      from: mockUSDCBalance,
      radix: .hex,
      decimals: Int(allUserAssets[1].decimals)
    ) ?? ""
    let mockUSDCAssetPrice: BraveWallet.AssetPrice = .init(
      fromAsset: allUserAssets[1].assetRatioId.lowercased(), toAsset: "usd",
      price: mockUSDCPrice, assetTimeframeChange: "-57.23")
    let mockSOLAssetPrice: BraveWallet.AssetPrice = .init(
      fromAsset: "sol", toAsset: "usd",
      price: mockSOLPrice, assetTimeframeChange: "-57.23")
    
    let keyringService = BraveWallet.TestKeyringService()
    keyringService._keyringInfo = { keyringId, completion in
      if keyringId == BraveWallet.KeyringId.default {
        let keyring: BraveWallet.KeyringInfo = .init(
          id: BraveWallet.KeyringId.default,
          isKeyringCreated: true,
          isLocked: false,
          isBackedUp: true,
          accountInfos: [.mockEthAccount, self.mockEthAccount2]
        )
        completion(keyring)
      } else {
        let keyring: BraveWallet.KeyringInfo = .init(
          id: BraveWallet.KeyringId.solana,
          isKeyringCreated: true,
          isLocked: false,
          isBackedUp: true,
          accountInfos: [.mockSolAccount]
        )
        completion(keyring)
      }
    }
    let rpcService = BraveWallet.TestJsonRpcService()
    rpcService._allNetworks = { coin, completion in
      completion(self.allNetworks.filter { $0.coin == coin })
    }
    rpcService._balance = { accountAddress, _, _, completion in
      completion(ethBalanceWei, .success, "") // eth balance for both eth accounts
    }
    rpcService._erc20TokenBalance = { contractAddress, accountAddress, _, completion in
      if accountAddress == self.mockEthAccount2.address {
        completion(usdcBalanceWei, .success, "") // usdc balance for `mockEthAccount2`
        return
      }
      completion("0", .success, "") // usdc balance for `mockEthAccount`
    }
    rpcService._solanaBalance = { accountAddress, chainId, completion in
      completion(mockSOLLamportBalance, .success, "") // sol balance
    }
    rpcService._splTokenAccountBalance = {_, _, _, completion in
      completion("\(mockNFTBalance)", UInt8(0), "\(mockNFTBalance)", .success, "") // sol nft balance
    }
    rpcService._solTokenMetadata = { tokenChainId, tokenMintAddress, completion in
      guard tokenChainId == self.allUserAssets[4].chainId,
            tokenMintAddress == self.allUserAssets[4].contractAddress else {
        completion("", "", .internalError, "")
        return
      }
      let metadata = """
      {
        "image": "\(mockNFTMetadata.imageURLString ?? "")",
        "name": "\(mockNFTMetadata.name ?? "")",
        "description": "\(mockNFTMetadata.description ?? "")"
      }
      """
      completion("", metadata, .success, "")
    }
    let walletService = BraveWallet.TestBraveWalletService()
    walletService._addObserver = { _ in }
    walletService._defaultBaseCurrency = { $0(CurrencyCode.usd.code) }
    let mockAssetManager = TestableWalletUserAssetManager()
    mockAssetManager._getAllUserAssetsInNetworkAssets = { _ in
      self.allUserAssetsInNetworkAssets
    }
    mockAssetManager._getAllVisibleAssetsInNetworkAssets = { networks in
      var result: [NetworkAssets] = []
      for network in networks {
        let visibleTokens = self.allUserAssets.filter {
          $0.chainId.caseInsensitiveCompare(network.chainId) == .orderedSame && $0.visible
        }
        if !visibleTokens.isEmpty {
          result.append(NetworkAssets(network: network, tokens: visibleTokens, sortOrder: 0))
        }
      }
      return result
    }
    let assetRatioService = BraveWallet.TestAssetRatioService()
    assetRatioService._price = { priceIds, _, _, completion in
      completion(true, [mockETHAssetPrice, mockUSDCAssetPrice, mockSOLAssetPrice])
    }

    let store = SelectAccountTokenStore(
      didSelect: { _, _ in },
      keyringService: keyringService,
      rpcService: rpcService,
      walletService: walletService,
      assetRatioService: assetRatioService,
      ipfsApi: TestIpfsAPI(),
      userAssetManager: mockAssetManager
    )

    XCTAssertTrue(store.accountSections.isEmpty)
    XCTAssertTrue(store.isHidingZeroBalances) // test expecting zero balances hidden
    
    let accountSectionsExpectation = expectation(description: "update-AccountSections")
    store.$accountSections
      .dropFirst() // initial
      .collect(4) // fetch accounts, fetch balance, fetch price, fetchNftMetadata
      .sink { accountSections in
        defer { accountSectionsExpectation.fulfill() }
        guard let accountSections = accountSections.last else {
          XCTFail("Unexpected test setup")
          return
        }
        XCTAssertEqual(accountSections.count, 3) // 2 eth accounts, 1 sol accounts
        
        XCTAssertEqual(accountSections[safe: 0]?.account, .mockEthAccount)
        XCTAssertEqual(accountSections[safe: 0]?.tokenBalances[safe: 0]?.token, self.allUserAssets[0]) // ETH
        XCTAssertEqual(accountSections[safe: 0]?.tokenBalances[safe: 1]?.token, self.allUserAssets[1]) // USDC
        
        XCTAssertEqual(accountSections[safe: 1]?.account, self.mockEthAccount2)
        XCTAssertEqual(accountSections[safe: 1]?.tokenBalances[safe: 0]?.token, self.allUserAssets[0]) // ETH
        XCTAssertEqual(accountSections[safe: 1]?.tokenBalances[safe: 1]?.token, self.allUserAssets[1]) // USDC (Goerli)
        
        XCTAssertEqual(accountSections[safe: 2]?.account, .mockSolAccount)
        XCTAssertEqual(accountSections[safe: 2]?.tokenBalances[safe: 0]?.token, self.allUserAssets[2]) // SOL
        XCTAssertEqual(accountSections[safe: 2]?.tokenBalances[safe: 1]?.token, self.allUserAssets[4]) // Solana NFT
        XCTAssertNil(accountSections[safe: 2]?.tokenBalances[safe: 2]) // `mockSpdToken` is not visible
      }.store(in: &cancellables)

    await store.update()
    await fulfillment(of: [accountSectionsExpectation], timeout: 1)
    
    // verify `filteredAccountSections` which get displayed in UI
    var accountSections = store.filteredAccountSections
    XCTAssertEqual(accountSections.count, 3) // 2 eth accounts, 1 sol accounts
    
    // Account 1
    XCTAssertEqual(accountSections[safe: 0]?.account, .mockEthAccount)
    XCTAssertEqual(accountSections[safe: 0]?.tokenBalances[safe: 0]?.token, self.allUserAssets[0]) // ETH
    XCTAssertEqual(accountSections[safe: 0]?.tokenBalances[safe: 0]?.network.chainId, BraveWallet.MainnetChainId)
    XCTAssertEqual(accountSections[safe: 0]?.tokenBalances[safe: 0]?.balance, mockETHBalance)
    XCTAssertEqual(accountSections[safe: 0]?.tokenBalances[safe: 0]?.price, "$2,741.75")
    XCTAssertNil(accountSections[safe: 0]?.tokenBalances[safe: 1]) // no USDC balance, token hidden
    
    // Ethereum Account 2
    XCTAssertEqual(accountSections[safe: 1]?.account, self.mockEthAccount2)
    XCTAssertEqual(accountSections[safe: 1]?.tokenBalances[safe: 0]?.token, self.allUserAssets[0]) // ETH
    XCTAssertEqual(accountSections[safe: 1]?.tokenBalances[safe: 0]?.network.chainId, BraveWallet.MainnetChainId)
    XCTAssertEqual(accountSections[safe: 1]?.tokenBalances[safe: 0]?.balance, mockETHBalance)
    XCTAssertEqual(accountSections[safe: 1]?.tokenBalances[safe: 0]?.price, "$2,741.75")
    XCTAssertEqual(accountSections[safe: 1]?.tokenBalances[safe: 1]?.token, self.allUserAssets[1]) // USDC
    XCTAssertEqual(accountSections[safe: 1]?.tokenBalances[safe: 1]?.network.chainId, BraveWallet.GoerliChainId)
    XCTAssertEqual(accountSections[safe: 1]?.tokenBalances[safe: 1]?.balance, mockUSDCBalance)
    XCTAssertEqual(accountSections[safe: 1]?.tokenBalances[safe: 1]?.price, "$4.00")
    let ethAccount2EthBalance = accountSections[safe: 1]?.tokenBalances[safe: 0]?.balance ?? 0
    let ethAccount2USDCBalance = accountSections[safe: 1]?.tokenBalances[safe: 1]?.balance ?? 0
    XCTAssertTrue(ethAccount2EthBalance < ethAccount2USDCBalance) // eth has smaller balance, but usdc on testnet
    
    // Solana Account 1
    XCTAssertEqual(accountSections[safe: 2]?.account, .mockSolAccount)
    XCTAssertEqual(accountSections[safe: 2]?.tokenBalances[safe: 0]?.token, self.allUserAssets[2]) // SOL
    XCTAssertEqual(accountSections[safe: 2]?.tokenBalances[safe: 0]?.network.chainId, BraveWallet.SolanaMainnet)
    XCTAssertEqual(accountSections[safe: 2]?.tokenBalances[safe: 0]?.balance, mockSOLBalance)
    XCTAssertEqual(accountSections[safe: 2]?.tokenBalances[safe: 0]?.price, "$775.30")
    XCTAssertEqual(accountSections[safe: 2]?.tokenBalances[safe: 1]?.token, self.allUserAssets[4]) // Solana NFT
    XCTAssertEqual(accountSections[safe: 2]?.tokenBalances[safe: 1]?.network.chainId, BraveWallet.SolanaTestnet)
    XCTAssertEqual(accountSections[safe: 2]?.tokenBalances[safe: 1]?.balance, mockNFTBalance)
    XCTAssertEqual(accountSections[safe: 2]?.tokenBalances[safe: 1]?.nftMetadata, mockNFTMetadata)
    
    // Test with zero balances shown
    store.isHidingZeroBalances = false
    accountSections = store.filteredAccountSections
    XCTAssertEqual(accountSections.count, 3) // 2 eth accounts, 1 sol accounts
    
    // Account 1
    XCTAssertEqual(accountSections[safe: 0]?.account, .mockEthAccount)
    XCTAssertEqual(accountSections[safe: 0]?.tokenBalances[safe: 0]?.token, self.allUserAssets[0]) // ETH
    XCTAssertEqual(accountSections[safe: 0]?.tokenBalances[safe: 1]?.token, self.allUserAssets[1]) // USDC
    
    // Ethereum Account 2
    XCTAssertEqual(accountSections[safe: 1]?.account, self.mockEthAccount2)
    XCTAssertEqual(accountSections[safe: 1]?.tokenBalances[safe: 0]?.token, self.allUserAssets[0]) // ETH
    XCTAssertEqual(accountSections[safe: 1]?.tokenBalances[safe: 1]?.token, self.allUserAssets[1]) // USDC (Goerli)
    
    // Solana Account 1
    XCTAssertEqual(accountSections[safe: 2]?.account, .mockSolAccount)
    XCTAssertEqual(accountSections[safe: 2]?.tokenBalances[safe: 0]?.token, self.allUserAssets[2]) // SOL
    XCTAssertEqual(accountSections[safe: 2]?.tokenBalances[safe: 1]?.token, self.allUserAssets[4]) // Solana NFT
  }
  
  func testNetworkFilter() {
    let keyringService = BraveWallet.TestKeyringService()
    let rpcService = BraveWallet.TestJsonRpcService()
    let walletService = BraveWallet.TestBraveWalletService()
    walletService._addObserver = { _ in }
    let assetRatioService = BraveWallet.TestAssetRatioService()

    let store = SelectAccountTokenStore(
      didSelect: { _, _ in },
      keyringService: keyringService,
      rpcService: rpcService,
      walletService: walletService,
      assetRatioService: assetRatioService,
      ipfsApi: TestIpfsAPI(),
      userAssetManager: TestableWalletUserAssetManager()
    )
    store.setupForTesting()
    XCTAssertTrue(store.accountSections.isEmpty)
    XCTAssertTrue(store.filteredAccountSections.isEmpty)
    store.accountSections = [
      .init(
        account: .mockEthAccount,
        tokenBalances: [
          .init(
            token: .previewToken,
            network: .mockMainnet,
            balance: 1
          ),
          .init(
            token: .previewDaiToken.copy(asVisibleAsset: true).then { $0.chainId = BraveWallet.GoerliChainId },
            network: .mockGoerli,
            balance: 2
          )
        ]
      ),
      .init(
        account: .mockSolAccount,
        tokenBalances: [
          .init(
            token: .mockSolToken,
            network: .mockSolana,
            balance: 3
          ),
          .init(
            token: .mockSpdToken.copy(asVisibleAsset: true).then { $0.chainId = BraveWallet.SolanaTestnet },
            network: .mockSolanaTestnet,
            balance: 4
          )
        ]
      )
    ]
    // all networks
    store.networkFilters = [
      .init(isSelected: true, model: .mockMainnet),
      .init(isSelected: true, model: .mockGoerli),
      .init(isSelected: true, model: .mockSolana),
      .init(isSelected: true, model: .mockSolanaTestnet)
    ]
    XCTAssertEqual(store.filteredAccountSections, store.accountSections)
    // Ethereum mainnet
    store.networkFilters = [.init(isSelected: true, model: .mockMainnet)]
    XCTAssertEqual(store.filteredAccountSections.count, 1)
    XCTAssertEqual(store.filteredAccountSections, [
      .init(
        account: .mockEthAccount,
        tokenBalances: [
          .init(
            token: .previewToken,
            network: .mockMainnet,
            balance: 1
          )
        ]
      )
    ])
    // Solana mainnet
    store.networkFilters = [.init(isSelected: true, model: .mockSolana)]
    XCTAssertEqual(store.filteredAccountSections.count, 1)
    XCTAssertEqual(store.filteredAccountSections, [
      .init(
        account: .mockSolAccount,
        tokenBalances: [
          .init(
            token: .mockSolToken,
            network: .mockSolana,
            balance: 3
          )
        ]
      )
    ])
  }
}
