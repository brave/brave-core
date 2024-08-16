// Copyright 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveCore
import Combine
import Preferences
import XCTest

@testable import BraveWallet

class SelectAccountTokenStoreTests: XCTestCase {

  private var cancellables: Set<AnyCancellable> = .init()

  private let allUserAssets: [BraveWallet.BlockchainToken] = [
    .previewToken.copy(asVisibleAsset: true),
    .mockUSDCToken.copy(asVisibleAsset: true).then { $0.chainId = BraveWallet.SepoliaChainId },
    .mockSolToken.copy(asVisibleAsset: true),
    .mockSpdToken.copy(asVisibleAsset: false),  // not visible
    .mockSolanaNFTToken.copy(asVisibleAsset: true).then { $0.chainId = BraveWallet.SolanaTestnet },
    .mockFilToken.copy(asVisibleAsset: true),
    .mockFilToken.copy(asVisibleAsset: true).then { $0.chainId = BraveWallet.FilecoinTestnet },
  ]
  private var allUserAssetsInNetworkAssets: [NetworkAssets] {
    [
      NetworkAssets(
        network: .mockMainnet,
        tokens: [allUserAssets[0]],
        sortOrder: 0
      ),
      NetworkAssets(
        network: .mockSepolia,
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
      ),
      NetworkAssets(
        network: .mockFilecoinMainnet,
        tokens: [allUserAssets[5]],
        sortOrder: 4
      ),
      NetworkAssets(
        network: .mockFilecoinTestnet,
        tokens: [allUserAssets[6]],
        sortOrder: 5
      ),
    ]
  }

  private let mockEthAccount2: BraveWallet.AccountInfo = .init(
    accountId: .init(
      coin: .eth,
      keyringId: BraveWallet.KeyringId.default,
      kind: .derived,
      address: "mock_eth_id_2",
      accountIndex: 0,
      uniqueKey: "mock_eth_id_2"
    ),
    address: "mock_eth_id_2",
    name: "Ethereum Account 2",
    hardware: nil
  )

  private let formatter = WalletAmountFormatter(decimalFormatStyle: .decimals(precision: 18))
  private let currencyFormatter = NumberFormatter().then { $0.numberStyle = .currency }

  /// Test `update()` will update `accountSections` for each account, and verify
  /// `filteredAccountSections` displays non-zero balance by default.
  @MainActor func testUpdate() async {
    let mockETHBalance: Double = 0.896
    let mockETHPrice: String = "3059.99"  // ETH value = $2741.75104
    let mockUSDCBalance: Double = 4
    let mockUSDCPrice: String = "1"  // USDC value = $4
    let mockSOLLamportBalance: UInt64 = 3_876_535_000  // ~3.8765 SOL
    let mockSOLBalance: Double = 3.8765  // lamports rounded
    let mockSOLPrice: String = "200"  // SOL value = $775.30
    let mockNFTBalance: Double = 1
    let mockNFTMetadata: BraveWallet.NftMetadata = .init(
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
    let mockFILBalance: Double = 1
    let mockFILPrice: String = "4.06"  // FIL value = $4.06

    let ethBalanceWei =
      formatter.weiString(
        from: mockETHBalance,
        radix: .hex,
        decimals: Int(allUserAssets[0].decimals)
      ) ?? ""
    let mockETHAssetPrice: BraveWallet.AssetPrice = .init(
      fromAsset: "eth",
      toAsset: "usd",
      price: mockETHPrice,
      assetTimeframeChange: "-57.23"
    )
    let usdcBalanceWei =
      formatter.weiString(
        from: mockUSDCBalance,
        radix: .hex,
        decimals: Int(allUserAssets[1].decimals)
      ) ?? ""
    let mockUSDCAssetPrice: BraveWallet.AssetPrice = .init(
      fromAsset: allUserAssets[1].assetRatioId.lowercased(),
      toAsset: "usd",
      price: mockUSDCPrice,
      assetTimeframeChange: "-57.23"
    )
    let mockSOLAssetPrice: BraveWallet.AssetPrice = .init(
      fromAsset: "sol",
      toAsset: "usd",
      price: mockSOLPrice,
      assetTimeframeChange: "-57.23"
    )
    let filBalanceWei =
      formatter.weiString(
        from: mockFILBalance,
        radix: .decimal,
        decimals: Int(allUserAssets[5].decimals)
      ) ?? ""
    let mockFILAssetPrice: BraveWallet.AssetPrice = .init(
      fromAsset: "fil",
      toAsset: "usd",
      price: mockFILPrice,
      assetTimeframeChange: "-57.23"
    )

    let keyringService = BraveWallet.TestKeyringService()
    keyringService._allAccounts = {
      $0(
        .init(
          accounts: [
            .mockEthAccount,
            self.mockEthAccount2,
            .mockSolAccount,
            .mockFilAccount,
            .mockFilTestnetAccount,
          ],
          selectedAccount: .mockEthAccount,
          ethDappSelectedAccount: .mockEthAccount,
          solDappSelectedAccount: .mockSolAccount
        )
      )
    }
    let rpcService = MockJsonRpcService()
    rpcService.hiddenNetworks.removeAll()
    rpcService._balance = { accountAddress, coin, _, completion in
      if coin == .eth {
        completion(ethBalanceWei, .success, "")  // eth balance for both eth accounts
      } else {  // .fil
        completion(filBalanceWei, .success, "")
      }
    }
    rpcService._erc20TokenBalance = { contractAddress, accountAddress, _, completion in
      if accountAddress == self.mockEthAccount2.address {
        completion(usdcBalanceWei, .success, "")  // usdc balance for `mockEthAccount2`
        return
      }
      completion("0", .success, "")  // usdc balance for `mockEthAccount`
    }
    rpcService._nftBalances = { _, nftIdentifiers, _, completion in
      completion([mockNFTBalance as NSNumber], "")
    }
    rpcService._nftMetadatas = { coin, _, completion in
      if coin == .sol {
        completion([mockNFTMetadata], "")
      } else {
        completion([], "Error")
      }
    }
    rpcService._solanaBalance = { accountAddress, chainId, completion in
      completion(mockSOLLamportBalance, .success, "")  // sol balance
    }
    let walletService = BraveWallet.TestBraveWalletService()
    walletService._addObserver = { _ in }
    walletService._defaultBaseCurrency = { $0(CurrencyCode.usd.code) }
    let mockAssetManager = TestableWalletUserAssetManager()
    mockAssetManager._getAllUserAssetsInNetworkAssets = { _, _ in
      self.allUserAssetsInNetworkAssets
    }
    mockAssetManager._getUserAssets = { networks, _ in
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
      completion(
        true,
        [mockETHAssetPrice, mockUSDCAssetPrice, mockSOLAssetPrice, mockFILAssetPrice]
      )
    }

    let bitcoinWalletService = BraveWallet.TestBitcoinWalletService()

    let store = SelectAccountTokenStore(
      didSelect: { _, _ in },
      keyringService: keyringService,
      rpcService: rpcService,
      walletService: walletService,
      assetRatioService: assetRatioService,
      bitcoinWalletService: bitcoinWalletService,
      ipfsApi: TestIpfsAPI(),
      userAssetManager: mockAssetManager
    )

    XCTAssertTrue(store.accountSections.isEmpty)
    XCTAssertTrue(store.isHidingZeroBalances)  // test expecting zero balances hidden

    let accountSectionsExpectation = expectation(description: "update-AccountSections")
    store.$accountSections
      .dropFirst()  // initial
      .collect(4)  // fetch accounts, fetch balance, fetch price, fetchNftMetadata
      .sink { accountSections in
        defer { accountSectionsExpectation.fulfill() }
        guard let accountSections = accountSections.last else {
          XCTFail("Unexpected test setup")
          return
        }

        XCTAssertEqual(accountSections.count, 5)  // 2 eth accounts, 1 sol accounts, 2 fil accounts

        // Account 1
        XCTAssertEqual(accountSections[safe: 0]?.account, .mockEthAccount)
        XCTAssertEqual(
          accountSections[safe: 0]?.tokenBalances[safe: 0]?.token,
          self.allUserAssets[0]
        )  // ETH
        XCTAssertEqual(
          accountSections[safe: 0]?.tokenBalances[safe: 0]?.network.chainId,
          BraveWallet.MainnetChainId
        )
        XCTAssertEqual(accountSections[safe: 0]?.tokenBalances[safe: 0]?.balance, mockETHBalance)
        XCTAssertEqual(accountSections[safe: 0]?.tokenBalances[safe: 0]?.price, "$2,741.75")
        // no USDC balance, token hidden
        XCTAssertNil(accountSections[safe: 0]?.tokenBalances[safe: 1])

        // Ethereum Account 2
        XCTAssertEqual(accountSections[safe: 1]?.account, self.mockEthAccount2)
        XCTAssertEqual(
          accountSections[safe: 1]?.tokenBalances[safe: 0]?.token,
          self.allUserAssets[0]
        )  // ETH
        XCTAssertEqual(
          accountSections[safe: 1]?.tokenBalances[safe: 0]?.network.chainId,
          BraveWallet.MainnetChainId
        )
        XCTAssertEqual(accountSections[safe: 1]?.tokenBalances[safe: 0]?.balance, mockETHBalance)
        XCTAssertEqual(accountSections[safe: 1]?.tokenBalances[safe: 0]?.price, "$2,741.75")
        XCTAssertEqual(
          accountSections[safe: 1]?.tokenBalances[safe: 1]?.token,
          self.allUserAssets[1]
        )  // USDC
        XCTAssertEqual(
          accountSections[safe: 1]?.tokenBalances[safe: 1]?.network.chainId,
          BraveWallet.SepoliaChainId
        )
        XCTAssertEqual(accountSections[safe: 1]?.tokenBalances[safe: 1]?.balance, mockUSDCBalance)
        XCTAssertEqual(accountSections[safe: 1]?.tokenBalances[safe: 1]?.price, "$4.00")
        let ethAccount2EthBalance = accountSections[safe: 1]?.tokenBalances[safe: 0]?.balance ?? 0
        let ethAccount2USDCBalance = accountSections[safe: 1]?.tokenBalances[safe: 1]?.balance ?? 0
        // eth has smaller balance, but usdc on testnet
        XCTAssertTrue(ethAccount2EthBalance < ethAccount2USDCBalance)

        // Solana Account 1
        XCTAssertEqual(accountSections[safe: 2]?.account, .mockSolAccount)
        XCTAssertEqual(
          accountSections[safe: 2]?.tokenBalances[safe: 0]?.token,
          self.allUserAssets[2]
        )  // SOL
        XCTAssertEqual(
          accountSections[safe: 2]?.tokenBalances[safe: 0]?.network.chainId,
          BraveWallet.SolanaMainnet
        )
        XCTAssertEqual(accountSections[safe: 2]?.tokenBalances[safe: 0]?.balance, mockSOLBalance)
        XCTAssertEqual(accountSections[safe: 2]?.tokenBalances[safe: 0]?.price, "$775.30")
        XCTAssertEqual(
          accountSections[safe: 2]?.tokenBalances[safe: 1]?.token,
          self.allUserAssets[4]
        )  // Solana NFT
        XCTAssertEqual(
          accountSections[safe: 2]?.tokenBalances[safe: 1]?.network.chainId,
          BraveWallet.SolanaTestnet
        )
        XCTAssertEqual(accountSections[safe: 2]?.tokenBalances[safe: 1]?.balance, mockNFTBalance)
        XCTAssertEqual(
          accountSections[safe: 2]?.tokenBalances[safe: 1]?.nftMetadata,
          mockNFTMetadata
        )

        // Filecoin account on mainnet
        XCTAssertEqual(accountSections[safe: 3]?.account, .mockFilAccount)
        XCTAssertEqual(
          accountSections[safe: 3]?.tokenBalances[safe: 0]?.token,
          self.allUserAssets[5]
        )  // FIL
        XCTAssertEqual(
          accountSections[safe: 3]?.tokenBalances[safe: 0]?.network.chainId,
          BraveWallet.FilecoinMainnet
        )
        XCTAssertEqual(accountSections[safe: 3]?.tokenBalances[safe: 0]?.balance, mockFILBalance)
        XCTAssertEqual(accountSections[safe: 3]?.tokenBalances[safe: 0]?.price, "$4.06")

        // Filecoin account on testnet
        XCTAssertEqual(accountSections[safe: 4]?.account, .mockFilTestnetAccount)
        XCTAssertEqual(
          accountSections[safe: 4]?.tokenBalances[safe: 0]?.token,
          self.allUserAssets[6]
        )  // FIL
        XCTAssertEqual(
          accountSections[safe: 4]?.tokenBalances[safe: 0]?.network.chainId,
          BraveWallet.FilecoinTestnet
        )
        XCTAssertEqual(accountSections[safe: 4]?.tokenBalances[safe: 0]?.balance, mockFILBalance)
        XCTAssertEqual(accountSections[safe: 4]?.tokenBalances[safe: 0]?.price, "$4.06")
      }.store(in: &cancellables)

    store.setup()
    await fulfillment(of: [accountSectionsExpectation], timeout: 1)
    cancellables.removeAll()

    let hidingZeroBalancesExpectation = expectation(description: "update-hidingZeroBalances")
    store.$accountSections
      .dropFirst()  // initial
      .sink { accountSections in
        defer { hidingZeroBalancesExpectation.fulfill() }

        XCTAssertEqual(accountSections.count, 5)  // 2 eth accounts, 1 sol accounts, 2 fil accounts

        // Account 1
        XCTAssertEqual(accountSections[safe: 0]?.account, .mockEthAccount)
        XCTAssertEqual(
          accountSections[safe: 0]?.tokenBalances[safe: 0]?.token,
          self.allUserAssets[0]
        )  // ETH
        XCTAssertEqual(
          accountSections[safe: 0]?.tokenBalances[safe: 1]?.token,
          self.allUserAssets[1]
        )  // USDC

        // Ethereum Account 2
        XCTAssertEqual(accountSections[safe: 1]?.account, self.mockEthAccount2)
        XCTAssertEqual(
          accountSections[safe: 1]?.tokenBalances[safe: 0]?.token,
          self.allUserAssets[0]
        )  // ETH
        XCTAssertEqual(
          accountSections[safe: 1]?.tokenBalances[safe: 1]?.token,
          self.allUserAssets[1]
        )  // USDC (Sepolia)

        // Solana Account 1
        XCTAssertEqual(accountSections[safe: 2]?.account, .mockSolAccount)
        XCTAssertEqual(
          accountSections[safe: 2]?.tokenBalances[safe: 0]?.token,
          self.allUserAssets[2]
        )  // SOL
        XCTAssertEqual(
          accountSections[safe: 2]?.tokenBalances[safe: 1]?.token,
          self.allUserAssets[4]
        )  // Solana NFT

        // Filecoin account on mainnet
        XCTAssertEqual(accountSections[safe: 3]?.account, .mockFilAccount)
        XCTAssertEqual(
          accountSections[safe: 3]?.tokenBalances[safe: 0]?.token,
          self.allUserAssets[5]
        )  // FIL

        // Filecoin account on testnet
        XCTAssertEqual(accountSections[safe: 4]?.account, .mockFilTestnetAccount)
        XCTAssertEqual(
          accountSections[safe: 4]?.tokenBalances[safe: 0]?.token,
          self.allUserAssets[6]
        )  // FIL
      }.store(in: &cancellables)

    // Test with zero balances shown
    store.isHidingZeroBalances = false
    await fulfillment(of: [hidingZeroBalancesExpectation], timeout: 1)
    cancellables.removeAll()

    let networkFilterExpectation = expectation(description: "update-networkFilter")
    store.$accountSections
      .dropFirst()  // initial
      .sink { accountSections in
        defer { networkFilterExpectation.fulfill() }

        XCTAssertEqual(accountSections.count, 2)  // 2 fil accounts
        // Filecoin account on mainnet
        XCTAssertEqual(accountSections[safe: 0]?.account, .mockFilAccount)
        XCTAssertEqual(
          accountSections[safe: 0]?.tokenBalances[safe: 0]?.token,
          self.allUserAssets[5]
        )  // FIL

        // Filecoin account on testnet
        XCTAssertEqual(accountSections[safe: 1]?.account, .mockFilTestnetAccount)
        XCTAssertEqual(
          accountSections[safe: 1]?.tokenBalances[safe: 0]?.token,
          self.allUserAssets[6]
        )  // FIL
      }.store(in: &cancellables)

    // Test with network filters applied (only Filecoin Mainnet, Filecoin Testnet selected)
    store.networkFilters = [
      .init(isSelected: false, model: .mockMainnet),
      .init(isSelected: false, model: .mockSepolia),
      .init(isSelected: false, model: .mockSolana),
      .init(isSelected: false, model: .mockSolanaTestnet),
      .init(isSelected: true, model: .mockFilecoinMainnet),
      .init(isSelected: true, model: .mockFilecoinTestnet),
    ]
    await fulfillment(of: [networkFilterExpectation], timeout: 1)
  }
}
