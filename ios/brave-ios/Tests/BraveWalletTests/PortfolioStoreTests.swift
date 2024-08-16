// Copyright 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveCore
import Combine
import Preferences
import XCTest

@testable import BraveWallet

@MainActor class PortfolioStoreTests: XCTestCase {

  private var cancellables: Set<AnyCancellable> = .init()
  private let currencyFormatter = NumberFormatter().then { $0.numberStyle = .currency }
  private var isLocked = true

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
    Preferences.Wallet.sortOrderFilter.reset()
    Preferences.Wallet.isHidingSmallBalancesFilter.reset()
    Preferences.Wallet.nonSelectedAccountsFilter.reset()
    Preferences.Wallet.nonSelectedNetworksFilter.reset()
    Preferences.Wallet.isBitcoinTestnetEnabled.reset()
  }

  // Accounts
  let ethAccount1: BraveWallet.AccountInfo = .mockEthAccount
  let ethAccount2 = (BraveWallet.AccountInfo.mockEthAccount.copy() as! BraveWallet.AccountInfo).then
  {
    $0.address = "mock_eth_id_2"
    $0.accountId.uniqueKey = $0.address
    $0.name = "Ethereum Account 2"
  }
  let solAccount1: BraveWallet.AccountInfo = .mockSolAccount
  let solAccount2 = (BraveWallet.AccountInfo.mockSolAccount.copy() as! BraveWallet.AccountInfo).then
  {
    $0.address = "mock_sol_id_2"
    $0.accountId.uniqueKey = $0.address
    $0.name = "Solana Account 2"
  }
  let filAccount1: BraveWallet.AccountInfo = .mockFilAccount
  let filAccount2 = (BraveWallet.AccountInfo.mockFilAccount.copy() as! BraveWallet.AccountInfo).then
  {
    $0.address = "mock_fil_id_2"
    $0.accountId.uniqueKey = $0.address
    $0.name = "Filecoin Account 2"
  }
  let filTestnetAccount: BraveWallet.AccountInfo = .mockFilTestnetAccount
  let btcAccount1: BraveWallet.AccountInfo = .mockBtcAccount
  let btcAccount2 = (BraveWallet.AccountInfo.mockBtcAccount.copy() as! BraveWallet.AccountInfo).then
  {
    $0.accountId.accountIndex = 1
    $0.accountId.uniqueKey = "4_0_0_1"
    $0.name = "Bitcoin Account 2"
  }
  let btcTestnetAccount: BraveWallet.AccountInfo = .mockBtcTestnetAccount

  // ETH Asset, balance, price, history
  let mockETHBalanceAccount1: Double = 0.896
  let mockETHPrice: String = "3059.99"  // ETH value = $2741.75104
  lazy var mockETHAssetPrice: BraveWallet.AssetPrice = .init(
    fromAsset: "eth",
    toAsset: "usd",
    price: mockETHPrice,
    assetTimeframeChange: "-57.23"
  )
  lazy var mockETHPriceHistory: [BraveWallet.AssetTimePrice] = [
    .init(date: Date(timeIntervalSinceNow: -1000), price: "3000.00"),
    .init(date: Date(), price: mockETHPrice),
  ]
  // USDC Asset, balance, price, history
  let mockUSDCBalanceAccount1: Double = 0.03
  let mockUSDCBalanceAccount2: Double = 0.01
  let mockUSDCPrice: String = "1"  // USDC total value = $0.04
  lazy var mockUSDCAssetPrice: BraveWallet.AssetPrice = .init(
    fromAsset: BraveWallet.BlockchainToken.mockUSDCToken.assetRatioId,
    toAsset: "usd",
    price: mockUSDCPrice,
    assetTimeframeChange: "-57.23"
  )
  lazy var mockUSDCPriceHistory: [BraveWallet.AssetTimePrice] = [
    .init(date: Date(timeIntervalSinceNow: -1000), price: "0.999"),
    .init(date: Date(), price: mockUSDCPrice),
  ]
  // SOL Asset, balance, price, history
  let mockSOLBalance: Double = 3.8765  // lamports rounded
  let mockSOLPrice: String = "200"  // SOL value = $775.30
  lazy var mockSOLAssetPrice: BraveWallet.AssetPrice = .init(
    fromAsset: "sol",
    toAsset: "usd",
    price: mockSOLPrice,
    assetTimeframeChange: "-57.23"
  )
  lazy var mockSOLPriceHistory: [BraveWallet.AssetTimePrice] = [
    .init(date: Date(timeIntervalSinceNow: -1000), price: mockSOLPrice),
    .init(date: Date(), price: "250.00"),
  ]

  // FIL Asset, balance, price, history on filecoin mainnet
  let mockFILBalanceAccount1: Double = 1
  let mockFILPrice: String = "4.00"  // FIL value on mainnet = $4.00
  lazy var mockFILAssetPrice: BraveWallet.AssetPrice = .init(
    fromAsset: "fil",
    toAsset: "usd",
    price: mockFILPrice,
    assetTimeframeChange: "-57.23"
  )
  lazy var mockFILPriceHistory: [BraveWallet.AssetTimePrice] = [
    .init(date: Date(timeIntervalSinceNow: -1000), price: "4.06"),
    .init(date: Date(), price: mockFILPrice),
  ]
  // FIL Asset, balance on filecoin testnet
  let mockFILBalanceTestnet: Double = 100  // FIL value on testnet = $400.00

  let mockAvailableBTCBalanceAccount1: Double = 0.00000005
  let mockPendingBTCBalanceAccount1: Double = 0.00000005
  lazy var mockBTCBalanceAccount1: Double =
    mockAvailableBTCBalanceAccount1 + mockPendingBTCBalanceAccount1
  let mockBTCPrice: String = "65726.00"
  lazy var mockBTCAssetPrice: BraveWallet.AssetPrice = .init(
    fromAsset: "btc",
    toAsset: "usd",
    price: mockBTCPrice,
    assetTimeframeChange: "4.00"
  )
  lazy var mockBTCPriceHistory: [BraveWallet.AssetTimePrice] = [
    .init(date: Date(timeIntervalSinceNow: -1000), price: "65326.00.06"),
    .init(date: Date(), price: mockBTCPrice),
  ]
  let mockBTCBalanceTestnet: Double = 0.00001

  var totalBalance: String {
    let totalEthBalanceValue: Double =
      (Double(mockETHAssetPrice.price) ?? 0) * mockETHBalanceAccount1
    var totalUSDCBalanceValue: Double = 0
    totalUSDCBalanceValue += (Double(mockUSDCAssetPrice.price) ?? 0) * mockUSDCBalanceAccount1
    totalUSDCBalanceValue += (Double(mockUSDCAssetPrice.price) ?? 0) * mockUSDCBalanceAccount2
    let totalSolBalanceValue: Double = (Double(mockSOLAssetPrice.price) ?? 0) * mockSOLBalance
    let totalFilBalanceValue: Double =
      (Double(mockFILAssetPrice.price) ?? 0) * mockFILBalanceAccount1
    let totalBtcBalanceValue: Double =
      (Double(mockBTCAssetPrice.price) ?? 0) * mockBTCBalanceAccount1
    let totalBalanceValue =
      totalEthBalanceValue + totalSolBalanceValue + totalUSDCBalanceValue
      + totalFilBalanceValue + totalBtcBalanceValue
    return currencyFormatter.formatAsFiat(totalBalanceValue) ?? ""
  }

  private func setupStore() -> PortfolioStore {
    let mockSOLLamportBalance: UInt64 = 3_876_535_000  // ~3.8765 SOL
    let formatter = WalletAmountFormatter(decimalFormatStyle: .decimals(precision: 18))

    // config filecoin on mainnet
    let mockFilUserAssets: [BraveWallet.BlockchainToken] = [
      BraveWallet.NetworkInfo.mockFilecoinMainnet.nativeToken.copy(asVisibleAsset: true)
    ]
    let mockFilBalanceInWei =
      formatter.weiString(
        from: mockFILBalanceAccount1,
        radix: .decimal,
        decimals: Int(BraveWallet.BlockchainToken.mockFilToken.decimals)
      ) ?? ""
    // config filecoin on testnet
    let mockFilTestnetUserAssets: [BraveWallet.BlockchainToken] = [
      BraveWallet.NetworkInfo.mockFilecoinTestnet.nativeToken.copy(asVisibleAsset: true)
    ]
    let mockFilTestnetBalanceInWei =
      formatter.weiString(
        from: mockFILBalanceTestnet,
        radix: .decimal,
        decimals: Int(BraveWallet.BlockchainToken.mockFilToken.decimals)
      ) ?? ""

    // config Solana
    let mockSolUserAssets: [BraveWallet.BlockchainToken] = [
      BraveWallet.NetworkInfo.mockSolana.nativeToken.copy(asVisibleAsset: true),
      .mockSpdToken,  // Verify non-visible assets not displayed #6386
    ]
    // config Ethereum
    let mockEthUserAssets: [BraveWallet.BlockchainToken] = [
      .previewToken.copy(asVisibleAsset: true),
      .previewDaiToken,  // Verify non-visible assets not displayed #6386
      .mockUSDCToken.copy(asVisibleAsset: true),
      .mockERC721NFTToken.copy(asVisibleAsset: true),  // Verify NFTs not used in Portfolio #7945
    ]
    let ethBalanceWei =
      formatter.weiString(
        from: mockETHBalanceAccount1,
        radix: .hex,
        decimals: Int(BraveWallet.BlockchainToken.previewToken.decimals)
      ) ?? ""
    let usdcAccount1BalanceWei =
      formatter.weiString(
        from: mockUSDCBalanceAccount1,
        radix: .hex,
        decimals: Int(BraveWallet.BlockchainToken.mockUSDCToken.decimals)
      ) ?? ""
    let usdcAccount2BalanceWei =
      formatter.weiString(
        from: mockUSDCBalanceAccount2,
        radix: .hex,
        decimals: Int(BraveWallet.BlockchainToken.mockUSDCToken.decimals)
      ) ?? ""

    let mockEthSepoliaUserAssets: [BraveWallet.BlockchainToken] = [
      BraveWallet.NetworkInfo.mockSepolia.nativeToken.copy(asVisibleAsset: true)
    ]

    // config bitcoin on mainnet
    let mockBtcUserAssets: [BraveWallet.BlockchainToken] = [
      BraveWallet.NetworkInfo.mockBitcoinMainnet.nativeToken.copy(asVisibleAsset: true)
    ]
    // config bitcoin on testnet
    let mockBtcTestnetUserAssets: [BraveWallet.BlockchainToken] = [
      BraveWallet.NetworkInfo.mockBitcoinTestnet.nativeToken.copy(asVisibleAsset: true)
    ]

    // setup test services
    let keyringService = BraveWallet.TestKeyringService()
    keyringService._addObserver = { _ in }
    keyringService._isLocked = { [weak self] completion in
      // unlocked would cause `update()` from call in `init` to be called prior to test being setup.g
      completion(self?.isLocked ?? true)
    }
    keyringService._allAccounts = {
      $0(
        .init(
          accounts: [
            self.ethAccount1, self.ethAccount2,
            self.solAccount1, self.solAccount2,
            self.filAccount1, self.filAccount2,
            self.filTestnetAccount,
            self.btcAccount1, self.btcAccount2,
            self.btcTestnetAccount,
          ],
          selectedAccount: self.ethAccount1,
          ethDappSelectedAccount: self.ethAccount1,
          solDappSelectedAccount: self.solAccount1
        )
      )
    }
    let rpcService = MockJsonRpcService()
    rpcService.hiddenNetworks = [.mockPolygon, .mockSolanaTestnet]
    rpcService._balance = { accountAddress, coin, chainId, completion in
      // eth balance
      if coin == .eth {
        if chainId == BraveWallet.MainnetChainId, accountAddress == self.ethAccount1.address {
          completion(ethBalanceWei, .success, "")
        } else {
          completion("", .success, "")
        }
      } else if coin == .fil {  // .fil
        if chainId == BraveWallet.FilecoinMainnet {
          if accountAddress == self.filAccount1.address {
            completion(mockFilBalanceInWei, .success, "")
          } else {
            completion("", .success, "")
          }
        } else {
          completion(mockFilTestnetBalanceInWei, .success, "")
        }
      } else {
        completion("", .success, "")
      }
    }
    rpcService._erc20TokenBalance = { contractAddress, accountAddress, _, completion in
      // usdc balance
      if accountAddress == self.ethAccount1.address {
        completion(usdcAccount1BalanceWei, .success, "")
      } else {
        completion(usdcAccount2BalanceWei, .success, "")
      }
    }
    rpcService._nftBalances = { _, _, _, completion in
      // should not be fetching NFT balance in Portfolio
      completion([], "Error Message")
    }
    rpcService._solanaBalance = { accountAddress, chainId, completion in
      // sol balance
      if accountAddress == self.solAccount1.address {
        completion(mockSOLLamportBalance, .success, "")
      } else {
        completion(0, .success, "")
      }
    }

    let walletService = BraveWallet.TestBraveWalletService()
    walletService._addObserver = { _ in }
    walletService._defaultBaseCurrency = { $0(CurrencyCode.usd.code) }
    let assetRatioService = BraveWallet.TestAssetRatioService()
    assetRatioService._price = { priceIds, _, _, completion in
      completion(
        true,
        [
          self.mockETHAssetPrice, self.mockUSDCAssetPrice, self.mockSOLAssetPrice,
          self.mockFILAssetPrice, self.mockBTCAssetPrice,
        ]
      )
    }
    assetRatioService._priceHistory = { priceId, _, _, completion in
      switch priceId {
      case "sol":
        completion(true, self.mockSOLPriceHistory)
      case "eth":
        completion(true, self.mockETHPriceHistory)
      case BraveWallet.BlockchainToken.mockUSDCToken.assetRatioId:
        completion(true, self.mockUSDCPriceHistory)
      case "fil":
        completion(true, self.mockFILPriceHistory)  // for both mainnet and testnet
      case "btc":
        completion(true, self.mockBTCPriceHistory)  // for both mainnet and testnet
      default:
        completion(false, [])
      }
    }

    let mockAssetManager = TestableWalletUserAssetManager()
    mockAssetManager._getUserAssets = { networks, _ in
      [
        NetworkAssets(
          network: .mockMainnet,
          tokens: mockEthUserAssets.filter(\.visible),
          sortOrder: 0
        ),
        NetworkAssets(
          network: .mockSolana,
          tokens: mockSolUserAssets.filter(\.visible),
          sortOrder: 1
        ),
        NetworkAssets(
          network: .mockSepolia,
          tokens: mockEthSepoliaUserAssets.filter(\.visible),
          sortOrder: 2
        ),
        NetworkAssets(
          network: .mockFilecoinMainnet,
          tokens: mockFilUserAssets.filter(\.visible),
          sortOrder: 3
        ),
        NetworkAssets(
          network: .mockFilecoinTestnet,
          tokens: mockFilTestnetUserAssets.filter(\.visible),
          sortOrder: 4
        ),
        NetworkAssets(
          network: .mockBitcoinMainnet,
          tokens: mockBtcUserAssets.filter(\.visible),
          sortOrder: 3
        ),
        NetworkAssets(
          network: .mockBitcoinTestnet,
          tokens: mockBtcTestnetUserAssets.filter(\.visible),
          sortOrder: 4
        ),
      ].filter { networkAsset in networks.contains(where: { $0 == networkAsset.network }) }
    }
    mockAssetManager._getUserAssets = { networks, _ in
      [
        NetworkAssets(
          network: .mockMainnet,
          tokens: mockEthUserAssets.filter(\.visible),
          sortOrder: 0
        ),
        NetworkAssets(
          network: .mockSolana,
          tokens: mockSolUserAssets.filter(\.visible),
          sortOrder: 1
        ),
        NetworkAssets(
          network: .mockSepolia,
          tokens: mockEthSepoliaUserAssets.filter(\.visible),
          sortOrder: 2
        ),
        NetworkAssets(
          network: .mockFilecoinMainnet,
          tokens: mockFilUserAssets.filter(\.visible),
          sortOrder: 3
        ),
        NetworkAssets(
          network: .mockFilecoinTestnet,
          tokens: mockFilTestnetUserAssets.filter(\.visible),
          sortOrder: 4
        ),
        NetworkAssets(
          network: .mockBitcoinMainnet,
          tokens: mockBtcUserAssets.filter(\.visible),
          sortOrder: 3
        ),
        NetworkAssets(
          network: .mockBitcoinTestnet,
          tokens: mockBtcTestnetUserAssets.filter(\.visible),
          sortOrder: 4
        ),
      ].filter { networkAsset in networks.contains(where: { $0 == networkAsset.network }) }
    }

    let btcAvailableBalanceInSatoshi =
      formatter.weiString(
        from: mockAvailableBTCBalanceAccount1,
        radix: .decimal,
        decimals: Int(BraveWallet.BlockchainToken.mockBTCToken.decimals)
      ) ?? ""
    let btcPendingBalanceInSatoshi =
      formatter.weiString(
        from: mockPendingBTCBalanceAccount1,
        radix: .decimal,
        decimals: Int(BraveWallet.BlockchainToken.mockBTCToken.decimals)
      ) ?? ""
    let btcBalanceInSatoshi =
      formatter.weiString(
        from: mockBTCBalanceAccount1,
        radix: .decimal,
        decimals: Int(BraveWallet.BlockchainToken.mockBTCToken.decimals)
      ) ?? ""
    let btcTestnetBalanceInSatoshi =
      formatter.weiString(
        from: mockBTCBalanceTestnet,
        radix: .decimal,
        decimals: Int(BraveWallet.BlockchainToken.mockBTCToken.decimals)
      ) ?? ""
    let bitcoinWalletService = BraveWallet.TestBitcoinWalletService()
    bitcoinWalletService._balance = { accountId, completion in
      if accountId.uniqueKey == self.btcAccount1.accountId.uniqueKey {
        completion(
          .init(
            totalBalance: UInt64(btcBalanceInSatoshi) ?? 0,
            availableBalance: UInt64(btcAvailableBalanceInSatoshi) ?? 0,
            pendingBalance: Int64(btcPendingBalanceInSatoshi) ?? 0,
            balances: [:]
          ),
          nil
        )
      } else if accountId.uniqueKey == self.btcAccount2.accountId.uniqueKey {
        completion(
          .init(
            totalBalance: 0,
            availableBalance: 0,
            pendingBalance: 0,
            balances: [:]
          ),
          nil
        )
      } else {
        completion(
          .init(
            totalBalance: UInt64(btcTestnetBalanceInSatoshi) ?? 0,
            availableBalance: UInt64(btcTestnetBalanceInSatoshi) ?? 0,
            pendingBalance: 0,
            balances: [:]
          ),
          nil
        )
      }
    }

    return PortfolioStore(
      keyringService: keyringService,
      rpcService: rpcService,
      walletService: walletService,
      assetRatioService: assetRatioService,
      blockchainRegistry: BraveWallet.TestBlockchainRegistry(),
      ipfsApi: TestIpfsAPI(),
      bitcoinWalletService: bitcoinWalletService,
      userAssetManager: mockAssetManager
    )
  }

  /// Test `update()` will fetch all visible user assets from all networks and display them sorted by their balance.
  func testUpdate() async {
    let store = setupStore()

    // MARK: Default update() Test
    let assetGroupsExpectation = expectation(description: "update-assetGroups")
    XCTAssertTrue(store.assetGroups.isEmpty)  // Initial state
    store.$assetGroups
      .dropFirst()
      .collect(2)
      .sink { assetGroups in
        defer { assetGroupsExpectation.fulfill() }
        XCTAssertEqual(assetGroups.count, 2)  // empty (no balance, price, history), populated

        guard let lastUpdatedAssetGroups = assetGroups.last else {
          XCTFail("Unexpected test result")
          return
        }
        XCTAssertEqual(lastUpdatedAssetGroups.count, 1)  // grouping by .none, so only 1 group
        guard let group = lastUpdatedAssetGroups.first else {
          XCTFail("Unexpected test result")
          return
        }

        // ETH on Ethereum mainnet, SOL on Solana mainnet, USDC on Ethereum mainnet, ETH on Sepolia, FIL on Filecoin mainnet, FIL on Filecoin testnet, BTC on Bitcoin mainnet. No BTC on Bitcoin testnet
        XCTAssertEqual(group.assets.count, 5)
        // ETH Mainnet (value ~= $2741.7510399999996)
        XCTAssertEqual(
          group.assets[safe: 0]?.token.symbol,
          BraveWallet.BlockchainToken.previewToken.symbol
        )
        XCTAssertEqual(
          group.assets[safe: 0]?.price,
          self.mockETHAssetPrice.price
        )
        XCTAssertEqual(
          group.assets[safe: 0]?.history,
          self.mockETHPriceHistory
        )
        XCTAssertEqual(
          group.assets[safe: 0]?.quantity,
          String(format: "%.04f", self.mockETHBalanceAccount1)
        )
        XCTAssertEqual(
          group.assets[safe: 0]?.btcBalances,
          [:]  // ETH so no btcBalances
        )

        // SOL (value = $775.3)
        XCTAssertEqual(
          group.assets[safe: 1]?.token.symbol,
          BraveWallet.BlockchainToken.mockSolToken.symbol
        )
        XCTAssertEqual(
          group.assets[safe: 1]?.price,
          self.mockSOLAssetPrice.price
        )
        XCTAssertEqual(
          group.assets[safe: 1]?.history,
          self.mockSOLPriceHistory
        )
        XCTAssertEqual(
          group.assets[safe: 1]?.quantity,
          String(format: "%.04f", self.mockSOLBalance)
        )

        // FIL (value $4.00) on mainnet
        XCTAssertEqual(
          group.assets[safe: 2]?.token.symbol,
          BraveWallet.BlockchainToken.mockFilToken.symbol
        )
        XCTAssertEqual(
          group.assets[safe: 2]?.price,
          self.mockFILAssetPrice.price
        )
        XCTAssertEqual(
          group.assets[safe: 2]?.history,
          self.mockFILPriceHistory
        )
        XCTAssertEqual(
          group.assets[safe: 2]?.quantity,
          String(format: "%.04f", self.mockFILBalanceAccount1)
        )
        XCTAssertEqual(
          group.assets[safe: 0]?.btcBalances,
          [:]  // FIL so no btcBalances
        )

        // USDC (value $0.04)
        XCTAssertEqual(
          group.assets[safe: 3]?.token.symbol,
          BraveWallet.BlockchainToken.mockUSDCToken.symbol
        )
        XCTAssertEqual(
          group.assets[safe: 3]?.price,
          self.mockUSDCAssetPrice.price
        )
        XCTAssertEqual(
          group.assets[safe: 3]?.history,
          self.mockUSDCPriceHistory
        )
        XCTAssertEqual(
          group.assets[safe: 3]?.quantity,
          String(format: "%.04f", self.mockUSDCBalanceAccount1 + self.mockUSDCBalanceAccount2)
        )
        // BTC (value $0.0065726) on mainnet
        XCTAssertEqual(
          group.assets[safe: 4]?.token.symbol,
          BraveWallet.BlockchainToken.mockBTCToken.symbol
        )
        XCTAssertEqual(
          group.assets[safe: 4]?.price,
          self.mockBTCAssetPrice.price
        )
        XCTAssertEqual(
          group.assets[safe: 4]?.history,
          self.mockBTCPriceHistory
        )
        XCTAssertEqual(
          group.assets[safe: 4]?.quantity,
          String(format: "%.04f", self.mockBTCBalanceAccount1)
        )
        XCTAssertEqual(
          group.assets[safe: 4]?.btcBalances,
          [
            self.btcAccount1.id: [
              .available: self.mockAvailableBTCBalanceAccount1,
              .pending: self.mockPendingBTCBalanceAccount1,
              .total: self.mockBTCBalanceAccount1,
            ],
            self.btcAccount2.id: [
              .available: 0,
              .pending: 0,
              .total: 0,
            ],
          ]
        )

        // ETH Sepolia (value = 0), hidden because test networks not selected by default
        // FIL Testnet (value = $400.00), hidden because test networks not selected by default
        // BIT Testnet (value = 0.00), hidden because Bitcoin testnet is disabled by default
        XCTAssertNil(group.assets[safe: 5])
      }
      .store(in: &cancellables)

    // test that `update()` will assign new value to `balance` publisher
    let balanceExpectation = expectation(description: "update-balance")
    store.$balance
      .dropFirst()
      .first()
      .sink { balance in
        defer { balanceExpectation.fulfill() }
        XCTAssertEqual(balance, self.totalBalance)
      }
      .store(in: &cancellables)
    // test that `update()` will assign new value to `balanceDifference` publisher
    let balanceDifferenceExpectation = expectation(description: "update-balanceDifference")
    store.$balanceDifference
      .dropFirst()
      .first()
      .sink { balanceDifference in
        defer { balanceDifferenceExpectation.fulfill() }
        XCTAssertEqual(
          balanceDifference,
          .init(
            priceDifference: "+$53.70",  //"+$53.69",
            percentageChange: "+1.55%",
            isBalanceUp: true
          )
        )
      }
      .store(in: &cancellables)
    // test that `update()` will update `isLoadingBalances` publisher
    let isLoadingBalancesExpectation = expectation(description: "update-isLoadingBalances")
    store.$isLoadingBalances
      .dropFirst()
      .collect(2)
      .first()
      .sink { isLoadingUpdates in
        defer { isLoadingBalancesExpectation.fulfill() }
        XCTAssertTrue(isLoadingUpdates[0])
        XCTAssertFalse(isLoadingUpdates[1])
      }
      .store(in: &cancellables)
    isLocked = false
    store.update()
    await fulfillment(
      of: [
        assetGroupsExpectation, balanceExpectation,
        balanceDifferenceExpectation, isLoadingBalancesExpectation,
      ],
      timeout: 1
    )
    cancellables.removeAll()
    let balanceDifferenceExpectationNan = expectation(description: "update-balanceDifferenceNan")
    store.$balanceDifference
      .dropFirst()
      .first()
      .sink { balanceDifference in
        defer { balanceDifferenceExpectationNan.fulfill() }
        XCTAssertEqual(
          balanceDifference,
          .init(
            priceDifference: "$0.00",
            percentageChange: "0.00%",
            isBalanceUp: false
          )
        )
      }
      .store(in: &cancellables)
    store.saveFilters(
      .init(
        groupBy: store.filters.groupBy,
        sortOrder: store.filters.sortOrder,
        isHidingSmallBalances: store.filters.isHidingSmallBalances,
        isHidingUnownedNFTs: store.filters.isHidingUnownedNFTs,
        isShowingNFTNetworkLogo: store.filters.isShowingNFTNetworkLogo,
        accounts: store.filters.accounts,
        networks: MockJsonRpcService.allKnownNetworks.map {
          // de-select all networks with balance
          .init(isSelected: $0.chainId == BraveWallet.SepoliaChainId, model: $0)
        }
      )
    )
    await fulfillment(
      of: [balanceDifferenceExpectationNan],
      timeout: 1
    )
  }

  /// Test `assetGroups` will be sorted to from smallest to highest fiat value when `sortOrder` filter is `valueAsc`.
  func filterSortHelper(bitcoinTestnetEnabled: Bool) async {
    Preferences.Wallet.isBitcoinTestnetEnabled.value = bitcoinTestnetEnabled
    let store = setupStore()
    let sortExpectation = expectation(description: "update-sortOrder")
    store.$assetGroups
      .dropFirst()
      .collect(2)
      .sink { assetGroups in
        defer { sortExpectation.fulfill() }
        XCTAssertEqual(assetGroups.count, 2)  // empty (no balance, price, history), populated
        guard let lastUpdatedAssetGroups = assetGroups.last else {
          XCTFail("Unexpected test result")
          return
        }
        XCTAssertEqual(lastUpdatedAssetGroups.count, 1)  // grouping by .none, so only 1 group
        guard let group = lastUpdatedAssetGroups.first else {
          XCTFail("Unexpected test result")
          return
        }
        // USDC on Ethereum mainnet, SOL on Solana mainnet, ETH on Ethereum mainnet, FIL on Filecoin mainnet, FIL on Filecoin testnet, BTC on Bitcoin mainnet. No BTC on Bitcoin testnet since Bitcoin tesnet is disabled by default
        let assetGroupNumber = bitcoinTestnetEnabled ? 8 : 7
        XCTAssertEqual(group.assets.count, assetGroupNumber)
        // ETH Sepolia (value = $0)
        XCTAssertEqual(
          group.assets[safe: 0]?.token.symbol,
          BraveWallet.BlockchainToken.previewToken.symbol
        )
        XCTAssertEqual(group.assets[safe: 0]?.quantity, String(format: "%.04f", 0))
        // BTC mainnet (value = $0.0065726)
        XCTAssertEqual(
          group.assets[safe: 1]?.token.symbol,
          BraveWallet.BlockchainToken.mockBTCToken.symbol
        )
        XCTAssertEqual(group.assets[safe: 1]?.quantity, String(format: "%.04f", 0))
        // USDC (value = $0.04)
        XCTAssertEqual(
          group.assets[safe: 2]?.token.symbol,
          BraveWallet.BlockchainToken.mockUSDCToken.symbol
        )
        XCTAssertEqual(
          group.assets[safe: 2]?.quantity,
          String(format: "%.04f", self.mockUSDCBalanceAccount1 + self.mockUSDCBalanceAccount2)
        )
        var offset = 2
        if bitcoinTestnetEnabled {
          offset += 1
          // BTC testnet (value = $0.65726)
          XCTAssertEqual(
            group.assets[safe: offset]?.token.symbol,
            BraveWallet.BlockchainToken.mockBTCToken.symbol
          )
          XCTAssertEqual(group.assets[safe: offset]?.quantity, String(format: "%.04f", 0))
        }

        // FIL (value = $4.00) on filecoin mainnet
        offset += 1
        XCTAssertEqual(
          group.assets[safe: offset]?.token.symbol,
          BraveWallet.BlockchainToken.mockFilToken.symbol
        )
        XCTAssertEqual(
          group.assets[safe: offset]?.quantity,
          String(format: "%.04f", self.mockFILBalanceAccount1)
        )
        // FIL (value = $400.00) on filecoin testnet
        offset += 1
        XCTAssertEqual(
          group.assets[safe: offset]?.token.symbol,
          BraveWallet.BlockchainToken.mockFilToken.symbol
        )
        XCTAssertEqual(
          group.assets[safe: offset]?.quantity,
          String(format: "%.04f", self.mockFILBalanceTestnet)
        )
        // SOL (value = $775.3)
        offset += 1
        XCTAssertEqual(
          group.assets[safe: offset]?.token.symbol,
          BraveWallet.BlockchainToken.mockSolToken.symbol
        )
        XCTAssertEqual(
          group.assets[safe: offset]?.quantity,
          String(format: "%.04f", self.mockSOLBalance)
        )
        // ETH Mainnet (value ~= $2741.7510399999996)
        offset += 1
        XCTAssertEqual(
          group.assets[safe: offset]?.token.symbol,
          BraveWallet.BlockchainToken.previewToken.symbol
        )
        XCTAssertEqual(
          group.assets[safe: offset]?.quantity,
          String(format: "%.04f", self.mockETHBalanceAccount1)
        )
      }.store(in: &cancellables)

    isLocked = false
    // change sort to ascending
    store.saveFilters(
      .init(
        groupBy: store.filters.groupBy,
        sortOrder: .valueAsc,
        isHidingSmallBalances: store.filters.isHidingSmallBalances,
        isHidingUnownedNFTs: store.filters.isHidingUnownedNFTs,
        isShowingNFTNetworkLogo: store.filters.isShowingNFTNetworkLogo,
        accounts: [
          ethAccount1, ethAccount2, solAccount1, solAccount2, filAccount1, filAccount2,
          filTestnetAccount, btcAccount1, btcAccount2, btcTestnetAccount,
        ].map {
          .init(isSelected: true, model: $0)
        },
        networks: MockJsonRpcService.allKnownNetworks.map {
          .init(isSelected: true, model: $0)
        }
      )
    )
    await fulfillment(of: [sortExpectation], timeout: 1)
    cancellables.removeAll()
  }

  func testFilterSort() async {
    await filterSortHelper(bitcoinTestnetEnabled: false)
  }

  func testFilterSortBitcoinTestnet() async {
    await filterSortHelper(bitcoinTestnetEnabled: true)
  }

  /// Test `assetGroups` will be filtered to remove small balances when `hideSmallBalances` filter is true.
  func testHideSmallBalances() async {
    let store = setupStore()
    let hideSmallBalancesExpectation = expectation(description: "update-hideSmallBalances")
    store.$assetGroups
      .dropFirst()
      .collect(2)
      .sink { assetGroups in
        defer { hideSmallBalancesExpectation.fulfill() }
        XCTAssertEqual(assetGroups.count, 2)  // empty (no balance, price, history), populated
        guard let lastUpdatedAssetGroups = assetGroups.last else {
          XCTFail("Unexpected test result")
          return
        }
        XCTAssertEqual(lastUpdatedAssetGroups.count, 1)  // grouping by .none, so only 1 group
        guard let group = lastUpdatedAssetGroups.first else {
          XCTFail("Unexpected test result")
          return
        }
        // ETH on Ethereum mainnet, SOL on Solana mainnet, FIL on Filecoin mainnet and testnet
        XCTAssertEqual(group.assets.count, 4)  // USDC, ETH Sepolia hidden for small balance
        // ETH Mainnet (value ~= $2741.7510399999996)
        XCTAssertEqual(
          group.assets[safe: 0]?.token.symbol,
          BraveWallet.BlockchainToken.previewToken.symbol
        )
        XCTAssertEqual(
          group.assets[safe: 0]?.quantity,
          String(format: "%.04f", self.mockETHBalanceAccount1)
        )
        // SOL (value = $775.3)
        XCTAssertEqual(
          group.assets[safe: 1]?.token.symbol,
          BraveWallet.BlockchainToken.mockSolToken.symbol
        )
        XCTAssertEqual(
          group.assets[safe: 1]?.quantity,
          String(format: "%.04f", self.mockSOLBalance)
        )
        // FIL (value = $400) on Filecoin testnet
        XCTAssertEqual(
          group.assets[safe: 2]?.token.symbol,
          BraveWallet.BlockchainToken.mockFilToken.symbol
        )
        XCTAssertEqual(
          group.assets[safe: 2]?.quantity,
          String(format: "%.04f", self.mockFILBalanceTestnet)
        )
        // FIL (value = $4) on Filecoin mainnet
        XCTAssertEqual(
          group.assets[safe: 3]?.token.symbol,
          BraveWallet.BlockchainToken.mockFilToken.symbol
        )
        XCTAssertEqual(
          group.assets[safe: 3]?.quantity,
          String(format: "%.04f", self.mockFILBalanceAccount1)
        )
        // USDC (value = $0.04), hidden
        // BTC (value = $0.006), hidden
        XCTAssertNil(group.assets[safe: 4])
      }.store(in: &cancellables)
    isLocked = false
    store.saveFilters(
      .init(
        groupBy: store.filters.groupBy,
        sortOrder: .valueDesc,
        isHidingSmallBalances: true,
        isHidingUnownedNFTs: store.filters.isHidingUnownedNFTs,
        isShowingNFTNetworkLogo: store.filters.isShowingNFTNetworkLogo,
        accounts: [
          ethAccount1, ethAccount2, solAccount1, solAccount2, filAccount1, filAccount2,
          filTestnetAccount, btcAccount1, btcAccount2,
        ].map {
          .init(isSelected: true, model: $0)
        },
        networks: MockJsonRpcService.allKnownNetworks.map {
          .init(isSelected: true, model: $0)
        }
      )
    )
    await fulfillment(of: [hideSmallBalancesExpectation], timeout: 1)
    cancellables.removeAll()
  }

  /// Test `assetGroups` will be filtered by accounts when `accounts` filter is has de-selected accounts.
  func filterAccountsHelper(bitcoinTestnetEnabled: Bool) async {
    // test without bitcoin testnet enabled
    Preferences.Wallet.isBitcoinTestnetEnabled.value = bitcoinTestnetEnabled
    let store = setupStore()
    let accountsExpectation = expectation(description: "update-accounts-bitcoin-testnet")
    store.$assetGroups
      .dropFirst()
      .collect(2)
      .sink { assetGroups in
        defer { accountsExpectation.fulfill() }
        XCTAssertEqual(assetGroups.count, 2)  // empty (no balance, price, history), populated
        guard let lastUpdatedAssetGroups = assetGroups.last else {
          XCTFail("Unexpected test result")
          return
        }
        XCTAssertEqual(lastUpdatedAssetGroups.count, 1)  // grouping by .none, so only 1 group
        guard let group = lastUpdatedAssetGroups.first else {
          XCTFail("Unexpected test result")
          return
        }
        // ETH on Ethereum mainnet, SOL on Solana mainnet, USDC on Ethereum mainnet, ETH on Sepolia, FIL on mainnet and testnet, BTC on mainnet and testnet
        let assetGroupNumber: Int = bitcoinTestnetEnabled ? 8 : 7
        XCTAssertEqual(group.assets.count, assetGroupNumber)
        // ETH Mainnet (value ~= $2741.7510399999996)
        XCTAssertEqual(
          group.assets[safe: 0]?.token.symbol,
          BraveWallet.BlockchainToken.previewToken.symbol
        )
        XCTAssertEqual(
          group.assets[safe: 0]?.quantity,
          String(format: "%.04f", self.mockETHBalanceAccount1)
        )
        // SOL (value = $775.3)
        XCTAssertEqual(
          group.assets[safe: 1]?.token.symbol,
          BraveWallet.BlockchainToken.mockSolToken.symbol
        )
        XCTAssertEqual(
          group.assets[safe: 1]?.quantity,
          String(format: "%.04f", self.mockSOLBalance)
        )
        // FIL (value = $400) on testnet
        XCTAssertEqual(
          group.assets[safe: 2]?.token.symbol,
          BraveWallet.BlockchainToken.mockFilToken.symbol
        )
        XCTAssertEqual(
          group.assets[safe: 2]?.quantity,
          String(format: "%.04f", self.mockFILBalanceTestnet)
        )
        // FIL (value = $4) on mainnet
        XCTAssertEqual(
          group.assets[safe: 3]?.token.symbol,
          BraveWallet.BlockchainToken.mockFilToken.symbol
        )
        XCTAssertEqual(
          group.assets[safe: 3]?.quantity,
          String(format: "%.04f", self.mockFILBalanceAccount1)
        )
        var offset: Int = 4
        if bitcoinTestnetEnabled {
          // BTC testnet (value = $0.65726)
          XCTAssertEqual(
            group.assets[safe: offset]?.token.symbol,
            BraveWallet.NetworkInfo.mockBitcoinTestnet.nativeToken.symbol
          )
          XCTAssertEqual(
            group.assets[safe: offset]?.quantity,
            String(format: "%.04f", self.mockBTCBalanceTestnet)
          )
          offset += 1
        }
        // USDC (value = $0.03, ethAccount2 hidden!)
        XCTAssertEqual(
          group.assets[safe: offset]?.token.symbol,
          BraveWallet.BlockchainToken.mockUSDCToken.symbol
        )
        XCTAssertEqual(
          group.assets[safe: offset]?.quantity,
          String(format: "%.04f", self.mockUSDCBalanceAccount1)
        )  // verify account 2 hidden
        offset += 1
        // BTC mainnet (value = $0.0006)
        XCTAssertEqual(
          group.assets[safe: offset]?.token.symbol,
          BraveWallet.NetworkInfo.mockBitcoinMainnet.nativeToken.symbol
        )
        XCTAssertEqual(
          group.assets[safe: offset]?.quantity,
          String(format: "%.04f", self.mockBTCBalanceAccount1)
        )
        offset += 1
        // ETH Sepolia (value = $0)
        XCTAssertEqual(
          group.assets[safe: offset]?.token.symbol,
          BraveWallet.NetworkInfo.mockSepolia.nativeToken.symbol
        )
        XCTAssertEqual(group.assets[safe: offset]?.quantity, String(format: "%.04f", 0))
      }.store(in: &cancellables)
    isLocked = false
    store.saveFilters(
      .init(
        groupBy: store.filters.groupBy,
        sortOrder: .valueDesc,
        isHidingSmallBalances: false,
        isHidingUnownedNFTs: store.filters.isHidingUnownedNFTs,
        isShowingNFTNetworkLogo: store.filters.isShowingNFTNetworkLogo,
        accounts: [
          ethAccount1, ethAccount2, solAccount1, solAccount2, filAccount1, filAccount2,
          filTestnetAccount, btcAccount1, btcAccount2, btcTestnetAccount,
        ].map {  // deselect ethAccount2
          .init(isSelected: $0 != ethAccount2, model: $0)
        },
        networks: MockJsonRpcService.allKnownNetworks.map {
          .init(isSelected: true, model: $0)
        }
      )
    )
    await fulfillment(of: [accountsExpectation], timeout: 1)
    cancellables.removeAll()
  }

  func testFilterAccounts() async {
    await filterAccountsHelper(bitcoinTestnetEnabled: false)
  }

  func testFilterAccountsWithBitcoinTestnet() async {
    await filterAccountsHelper(bitcoinTestnetEnabled: true)
  }

  /// Test `assetGroups` will be filtered by network when `networks` filter is has de-selected networks.
  func testFilterNetworks() async {
    let store = setupStore()
    let networksExpectation = expectation(description: "update-networks")
    store.$assetGroups
      .dropFirst()
      .collect(2)
      .sink { assetGroups in
        defer { networksExpectation.fulfill() }
        XCTAssertEqual(assetGroups.count, 2)  // empty (no balance, price, history), populated
        guard let lastUpdatedAssetGroups = assetGroups.last else {
          XCTFail("Unexpected test result")
          return
        }
        XCTAssertEqual(lastUpdatedAssetGroups.count, 1)  // grouping by .none, so only 1 group
        guard let group = lastUpdatedAssetGroups.first else {
          XCTFail("Unexpected test result")
          return
        }
        // ETH on Ethereum mainnet, USDC on Ethereum mainnet, ETH on Sepolia, FIL on mainnet and testnet BTC on mainnet and testnet
        XCTAssertEqual(group.assets.count, 5)
        // ETH Mainnet (value ~= $2741.7510399999996)
        XCTAssertEqual(
          group.assets[safe: 0]?.token.symbol,
          BraveWallet.BlockchainToken.previewToken.symbol
        )
        XCTAssertEqual(
          group.assets[safe: 0]?.quantity,
          String(format: "%.04f", self.mockETHBalanceAccount1)
        )
        // FIL (value = $400) on testnet
        XCTAssertEqual(
          group.assets[safe: 1]?.token.symbol,
          BraveWallet.BlockchainToken.mockFilToken.symbol
        )
        XCTAssertEqual(
          group.assets[safe: 1]?.quantity,
          String(format: "%.04f", self.mockFILBalanceTestnet)
        )
        // FIL (value = $4) on mainnet
        XCTAssertEqual(
          group.assets[safe: 2]?.token.symbol,
          BraveWallet.BlockchainToken.mockFilToken.symbol
        )
        XCTAssertEqual(
          group.assets[safe: 2]?.quantity,
          String(format: "%.04f", self.mockFILBalanceAccount1)
        )
        // USDC (value = $0.04)
        XCTAssertEqual(
          group.assets[safe: 3]?.token.symbol,
          BraveWallet.BlockchainToken.mockUSDCToken.symbol
        )
        XCTAssertEqual(
          group.assets[safe: 3]?.quantity,
          String(format: "%.04f", self.mockUSDCBalanceAccount1 + self.mockUSDCBalanceAccount2)
        )
        // ETH Sepolia (value = $0)
        XCTAssertEqual(
          group.assets[safe: 4]?.token.symbol,
          BraveWallet.NetworkInfo.mockSepolia.nativeToken.symbol
        )
        XCTAssertEqual(group.assets[safe: 4]?.quantity, String(format: "%.04f", 0))
        // SOL (value = $0, SOL networks hidden)
        // FIL networks hidden
        // BTC networks hidden
        XCTAssertNil(group.assets[safe: 5])

        // Verify NFTs not used in Portfolio #7945
        let noAssetsAreNFTs = lastUpdatedAssetGroups.flatMap(\.assets).allSatisfy({
          !($0.token.isNft || $0.token.isErc721)
        })
        XCTAssertTrue(noAssetsAreNFTs)
      }.store(in: &cancellables)
    isLocked = false
    store.saveFilters(
      .init(
        groupBy: store.filters.groupBy,
        sortOrder: .valueDesc,
        isHidingSmallBalances: false,
        isHidingUnownedNFTs: store.filters.isHidingUnownedNFTs,
        isShowingNFTNetworkLogo: store.filters.isShowingNFTNetworkLogo,
        accounts: [
          ethAccount1, ethAccount2, solAccount1, solAccount2, filAccount1, filAccount2,
          filTestnetAccount, btcAccount1, btcAccount2, btcTestnetAccount,
        ].map {
          .init(isSelected: true, model: $0)
        },
        networks: MockJsonRpcService.allKnownNetworks.map {
          // only select Ethereum networks
          .init(isSelected: $0.coin == .eth || $0.coin == .fil, model: $0)
        }
      )
    )
    await fulfillment(of: [networksExpectation], timeout: 1)
  }

  /// Test `assetGroups` will be grouped by account when `GroupBy` filter is assigned `.account`.
  /// Additionally, test de-selecting/hiding one of the available accounts.
  func groupByAccountsHelper(bitcoinTestnetEnabled: Bool) async {
    Preferences.Wallet.isBitcoinTestnetEnabled.value = bitcoinTestnetEnabled
    let store = setupStore()
    let assetGroupsExpectation = expectation(description: "update-assetGroups")
    XCTAssertTrue(store.assetGroups.isEmpty)  // Initial state
    store.$assetGroups
      .dropFirst()
      .collect(2)
      .sink { assetGroups in
        defer { assetGroupsExpectation.fulfill() }
        XCTAssertEqual(assetGroups.count, 2)  // empty (no balance, price, history), populated
        guard let lastUpdatedAssetGroups = assetGroups.last else {
          XCTFail("Unexpected test result")
          return
        }
        // grouping by .account; eth has 2 accounts, sol has 2 accounts, fil has 3 accounts, btc has 3 accounts (one tesnet)
        let assetGroupNumber = bitcoinTestnetEnabled ? 10 : 9
        XCTAssertEqual(lastUpdatedAssetGroups.count, assetGroupNumber)

        var btcTestnetIndex = 0
        var ethAccount2Index = 4
        var btcAccount1Index = 5
        var solAccount2Index = 6
        var btcAccount2Index = 7
        var filAccount2Index = 8
        if bitcoinTestnetEnabled {
          btcTestnetIndex = 4
          ethAccount2Index = 5
          btcAccount1Index = 6
          solAccount2Index = 7
          btcAccount2Index = 8
          filAccount2Index = 9
        }
        guard let ethAccount1Group = lastUpdatedAssetGroups[safe: 0],
          let solAccount1Group = lastUpdatedAssetGroups[safe: 1],
          let filTestnetAccountGroup = lastUpdatedAssetGroups[safe: 2],
          let filAccount1Group = lastUpdatedAssetGroups[safe: 3],
          let ethAccount2Group = lastUpdatedAssetGroups[safe: ethAccount2Index],
          let btcAccount1Group = lastUpdatedAssetGroups[safe: btcAccount1Index],
          let solAccount2Group = lastUpdatedAssetGroups[safe: solAccount2Index],
          let btcAccount2Group = lastUpdatedAssetGroups[safe: btcAccount2Index],
          let filAccount2Group = lastUpdatedAssetGroups[safe: filAccount2Index]
        else {
          XCTFail("Unexpected test result")
          return
        }

        if bitcoinTestnetEnabled {
          guard let btcTestnetAccountGroup = lastUpdatedAssetGroups[safe: btcTestnetIndex]
          else {
            XCTFail("Unexpected test result")
            return
          }

          XCTAssertEqual(btcTestnetAccountGroup.groupType, .account(self.btcTestnetAccount))
          XCTAssertEqual(btcTestnetAccountGroup.assets.count, 1)
          // BTC (value = $0)
          XCTAssertEqual(
            btcTestnetAccountGroup.assets[safe: 0]?.token.symbol,
            BraveWallet.BlockchainToken.mockBTCToken.symbol
          )
          XCTAssertEqual(
            btcTestnetAccountGroup.assets[safe: 0]?.quantity,
            String(format: "%.04f", 0)
          )
          XCTAssertEqual(
            btcTestnetAccountGroup.assets[safe: 0]?.btcBalances,
            [
              self.btcTestnetAccount.id: [
                .available: self.mockBTCBalanceTestnet,
                .pending: 0,
                .total: self.mockBTCBalanceTestnet,
              ]
            ]
          )
        }

        XCTAssertEqual(ethAccount1Group.groupType, .account(self.ethAccount1))
        XCTAssertEqual(ethAccount1Group.assets.count, 3)  // ETH Mainnet, USDC, ETH Sepolia
        // ETH Mainnet (value ~= $2741.7510399999996)
        XCTAssertEqual(
          ethAccount1Group.assets[safe: 0]?.token.symbol,
          BraveWallet.BlockchainToken.previewToken.symbol
        )
        XCTAssertEqual(
          ethAccount1Group.assets[safe: 0]?.quantity,
          String(format: "%.04f", self.mockETHBalanceAccount1)
        )
        // USDC (value = $0.03)
        XCTAssertEqual(
          ethAccount1Group.assets[safe: 1]?.token.symbol,
          BraveWallet.BlockchainToken.mockUSDCToken.symbol
        )
        XCTAssertEqual(
          ethAccount1Group.assets[safe: 1]?.quantity,
          String(format: "%.04f", self.mockUSDCBalanceAccount1)
        )
        // ETH Sepolia (value = $0)
        XCTAssertEqual(
          ethAccount1Group.assets[safe: 2]?.token.symbol,
          BraveWallet.NetworkInfo.mockSepolia.nativeToken.symbol
        )
        XCTAssertEqual(ethAccount1Group.assets[safe: 2]?.quantity, String(format: "%.04f", 0))

        XCTAssertEqual(solAccount1Group.groupType, .account(self.solAccount1))
        XCTAssertEqual(solAccount1Group.assets.count, 1)
        // SOL (value = $775.3)
        XCTAssertEqual(
          solAccount1Group.assets[safe: 0]?.token.symbol,
          BraveWallet.BlockchainToken.mockSolToken.symbol
        )
        XCTAssertEqual(
          solAccount1Group.assets[safe: 0]?.quantity,
          String(format: "%.04f", self.mockSOLBalance)
        )

        XCTAssertEqual(filTestnetAccountGroup.groupType, .account(self.filTestnetAccount))
        XCTAssertEqual(filTestnetAccountGroup.assets.count, 1)
        // FIL (value = $400)
        XCTAssertEqual(
          filTestnetAccountGroup.assets[safe: 0]?.token.symbol,
          BraveWallet.BlockchainToken.mockFilToken.symbol
        )
        XCTAssertEqual(
          filTestnetAccountGroup.assets[safe: 0]?.quantity,
          String(format: "%.04f", self.mockFILBalanceTestnet)
        )

        XCTAssertEqual(filAccount1Group.groupType, .account(self.filAccount1))
        XCTAssertEqual(filAccount1Group.assets.count, 1)
        // FIL (value = $4)
        XCTAssertEqual(
          filAccount1Group.assets[safe: 0]?.token.symbol,
          BraveWallet.BlockchainToken.mockFilToken.symbol
        )
        XCTAssertEqual(
          filAccount1Group.assets[safe: 0]?.quantity,
          String(format: "%.04f", self.mockFILBalanceAccount1)
        )

        XCTAssertEqual(ethAccount2Group.groupType, .account(self.ethAccount2))
        XCTAssertEqual(ethAccount2Group.assets.count, 3)  // ETH Mainnet, USDC, ETH Sepolia
        // USDC (value $0.01)
        XCTAssertEqual(
          ethAccount2Group.assets[safe: 0]?.token.symbol,
          BraveWallet.BlockchainToken.mockUSDCToken.symbol
        )
        XCTAssertEqual(
          ethAccount2Group.assets[safe: 0]?.quantity,
          String(format: "%.04f", self.mockUSDCBalanceAccount2)
        )
        // ETH Mainnet (value = $0)
        XCTAssertEqual(
          ethAccount2Group.assets[safe: 1]?.token.symbol,
          BraveWallet.BlockchainToken.previewToken.symbol
        )
        XCTAssertEqual(ethAccount2Group.assets[safe: 1]?.quantity, String(format: "%.04f", 0))
        // ETH Sepolia (value = $0)
        XCTAssertEqual(
          ethAccount2Group.assets[safe: 2]?.token.symbol,
          BraveWallet.NetworkInfo.mockSepolia.nativeToken.symbol
        )
        XCTAssertEqual(ethAccount2Group.assets[safe: 2]?.quantity, String(format: "%.04f", 0))

        XCTAssertEqual(btcAccount1Group.assets[safe: 0]?.quantity, String(format: "%.04f", 0))
        XCTAssertEqual(btcAccount1Group.groupType, .account(self.btcAccount1))
        XCTAssertEqual(btcAccount1Group.assets.count, 1)
        // BTC (value = $0.0065726)
        XCTAssertEqual(
          btcAccount1Group.assets[safe: 0]?.token.symbol,
          BraveWallet.BlockchainToken.mockBTCToken.symbol
        )

        XCTAssertEqual(solAccount2Group.groupType, .account(self.solAccount2))
        XCTAssertEqual(solAccount2Group.assets.count, 1)
        // SOL (value = $0)
        XCTAssertEqual(
          solAccount2Group.assets[safe: 0]?.token.symbol,
          BraveWallet.BlockchainToken.mockSolToken.symbol
        )
        XCTAssertEqual(solAccount2Group.assets[safe: 0]?.quantity, String(format: "%.04f", 0))

        XCTAssertEqual(btcAccount2Group.assets[safe: 0]?.quantity, String(format: "%.04f", 0))
        XCTAssertEqual(btcAccount2Group.groupType, .account(self.btcAccount2))
        XCTAssertEqual(btcAccount2Group.assets.count, 1)
        // BTC (value = $0)
        XCTAssertEqual(
          btcAccount2Group.assets[safe: 0]?.token.symbol,
          BraveWallet.BlockchainToken.mockBTCToken.symbol
        )
        XCTAssertEqual(btcAccount2Group.assets[safe: 0]?.quantity, String(format: "%.04f", 0))

        XCTAssertEqual(filAccount2Group.groupType, .account(self.filAccount2))
        XCTAssertEqual(filAccount2Group.assets.count, 1)
        // FIL (value = $0)
        XCTAssertEqual(
          filAccount2Group.assets[safe: 0]?.token.symbol,
          BraveWallet.BlockchainToken.mockFilToken.symbol
        )

        // Verify NFTs not used in Portfolio #7945
        let noAssetsAreNFTs = lastUpdatedAssetGroups.flatMap(\.assets).allSatisfy({
          !($0.token.isNft || $0.token.isErc721)
        })
        XCTAssertTrue(noAssetsAreNFTs)
      }
      .store(in: &cancellables)
    isLocked = false
    store.saveFilters(
      .init(
        groupBy: .accounts,
        sortOrder: store.filters.sortOrder,
        isHidingSmallBalances: store.filters.isHidingSmallBalances,
        isHidingUnownedNFTs: store.filters.isHidingUnownedNFTs,
        isShowingNFTNetworkLogo: store.filters.isShowingNFTNetworkLogo,
        accounts: [
          ethAccount1, ethAccount2, solAccount1, solAccount2, filAccount1, filAccount2,
          filTestnetAccount, btcAccount1, btcAccount2, btcTestnetAccount,
        ].map {
          .init(isSelected: true, model: $0)
        },
        networks: MockJsonRpcService.allKnownNetworks.map {
          .init(isSelected: true, model: $0)
        }
      )
    )
    await fulfillment(of: [assetGroupsExpectation], timeout: 1)
    cancellables.removeAll()
    // test hiding an account & hiding groups with small balances
    let accountsExpectation = expectation(description: "update-accounts")
    store.$assetGroups
      .dropFirst()
      .collect(2)
      .sink { assetGroups in
        defer { accountsExpectation.fulfill() }
        XCTAssertEqual(assetGroups.count, 2)  // empty (no balance, price, history), populated
        guard let lastUpdatedAssetGroups = assetGroups.last else {
          XCTFail("Unexpected test result")
          return
        }

        // grouping by .account; 1 for each of the 2 accounts selected accounts
        let groupNumber = bitcoinTestnetEnabled ? 5 : 4
        XCTAssertEqual(lastUpdatedAssetGroups.count, groupNumber)
        guard let ethAccount1Group = lastUpdatedAssetGroups[safe: 0],
          let solAccountGroup = lastUpdatedAssetGroups[safe: 1],
          let filTestnetAccountGroup = lastUpdatedAssetGroups[safe: 2],
          let filAccount1Group = lastUpdatedAssetGroups[safe: 3]
        else {
          XCTFail("Unexpected test result")
          return
        }
        XCTAssertEqual(ethAccount1Group.groupType, .account(self.ethAccount1))
        // ETH Mainnet (USDC, ETH Sepolia hidden for small balance)
        XCTAssertEqual(ethAccount1Group.assets.count, 1)
        // ETH Mainnet (value ~= $2741.7510399999996)
        XCTAssertEqual(
          ethAccount1Group.assets[safe: 0]?.token.symbol,
          BraveWallet.BlockchainToken.previewToken.symbol
        )
        XCTAssertEqual(
          ethAccount1Group.assets[safe: 0]?.quantity,
          String(format: "%.04f", self.mockETHBalanceAccount1)
        )
        // USDC (value $0.03)
        XCTAssertNil(ethAccount1Group.assets[safe: 1])

        XCTAssertEqual(solAccountGroup.groupType, .account(.mockSolAccount))
        XCTAssertEqual(solAccountGroup.assets.count, 1)
        // SOL (value = $775.3)
        XCTAssertEqual(
          solAccountGroup.assets[safe: 0]?.token.symbol,
          BraveWallet.BlockchainToken.mockSolToken.symbol
        )
        XCTAssertEqual(
          solAccountGroup.assets[safe: 0]?.quantity,
          String(format: "%.04f", self.mockSOLBalance)
        )

        XCTAssertEqual(filTestnetAccountGroup.groupType, .account(self.filTestnetAccount))
        XCTAssertEqual(filTestnetAccountGroup.assets.count, 1)
        // FIL (value = $400)
        XCTAssertEqual(
          filTestnetAccountGroup.assets[safe: 0]?.token.symbol,
          BraveWallet.BlockchainToken.mockFilToken.symbol
        )
        XCTAssertEqual(
          filTestnetAccountGroup.assets[safe: 0]?.quantity,
          String(format: "%.04f", self.mockFILBalanceTestnet)
        )

        XCTAssertEqual(filAccount1Group.groupType, .account(self.filAccount1))
        XCTAssertEqual(filAccount1Group.assets.count, 1)
        // FIL (value = $4)
        XCTAssertEqual(
          filAccount1Group.assets[safe: 0]?.token.symbol,
          BraveWallet.BlockchainToken.mockFilToken.symbol
        )
        XCTAssertEqual(
          filAccount1Group.assets[safe: 0]?.quantity,
          String(format: "%.04f", self.mockFILBalanceAccount1)
        )

        if bitcoinTestnetEnabled {
          guard let btcTestnetAccountGroup = lastUpdatedAssetGroups[safe: 4]
          else {
            XCTFail("Unexpected test result")
            return
          }

          XCTAssertEqual(btcTestnetAccountGroup.groupType, .account(self.btcTestnetAccount))
          XCTAssertEqual(btcTestnetAccountGroup.assets.count, 1)
          // BTC on testnet (value = $0.65726)
          XCTAssertEqual(
            btcTestnetAccountGroup.assets[safe: 0]?.token.symbol,
            BraveWallet.BlockchainToken.mockBTCToken.symbol
          )
          XCTAssertEqual(
            btcTestnetAccountGroup.assets[safe: 0]?.quantity,
            String(format: "%.04f", self.mockBTCBalanceTestnet)
          )
          XCTAssertEqual(
            btcTestnetAccountGroup.assets[safe: 0]?.btcBalances,
            [
              self.btcTestnetAccount.id: [
                .available: self.mockBTCBalanceTestnet,
                .pending: 0,
                .total: self.mockBTCBalanceTestnet,
              ]
            ]
          )
        }

        // ethAccount2 hidden as it's de-selected, solAccount2 hidden for small balance, filAccount2 hidden for small balance, btcAccount1/btcAccount2 hidden for small balance
        let lastIndex = bitcoinTestnetEnabled ? 5 : 4
        XCTAssertNil(lastUpdatedAssetGroups[safe: lastIndex])
      }
      .store(in: &cancellables)
    store.saveFilters(
      .init(
        groupBy: .accounts,
        sortOrder: store.filters.sortOrder,
        isHidingSmallBalances: true,
        isHidingUnownedNFTs: store.filters.isHidingUnownedNFTs,
        isShowingNFTNetworkLogo: store.filters.isShowingNFTNetworkLogo,
        accounts: [
          ethAccount1, ethAccount2, solAccount1, solAccount2, filAccount1, filAccount2,
          filTestnetAccount, btcAccount1, btcAccount2, btcTestnetAccount,
        ].map {
          .init(isSelected: $0 != ethAccount2, model: $0)
        },
        networks: MockJsonRpcService.allKnownNetworks.map {
          .init(isSelected: true, model: $0)
        }
      )
    )
    await fulfillment(of: [accountsExpectation], timeout: 1)
  }

  func testGroupByAccounts() async {
    await groupByAccountsHelper(bitcoinTestnetEnabled: false)
  }

  func testGroupByAccountsBitcoinTestnet() async {
    await groupByAccountsHelper(bitcoinTestnetEnabled: true)
  }

  /// Test `assetGroups` will be grouped by network when `GroupBy` filter is assigned `.network`.
  /// Additionally, test de-selecting/hiding one of the available networks.
  func groupByNetworksHelper(bitcoinTestnetEnabled: Bool) async {
    Preferences.Wallet.isBitcoinTestnetEnabled.value = bitcoinTestnetEnabled
    let store = setupStore()
    let assetGroupsExpectation = expectation(description: "update-assetGroups")
    XCTAssertTrue(store.assetGroups.isEmpty)  // Initial state
    store.$assetGroups
      .dropFirst()
      .collect(2)
      .sink { assetGroups in
        defer { assetGroupsExpectation.fulfill() }
        XCTAssertEqual(assetGroups.count, 2)  // empty (no balance, price, history), populated
        guard let lastUpdatedAssetGroups = assetGroups.last else {
          XCTFail("Unexpected test result")
          return
        }

        // grouping by .network; 1 for each of the 2 networks (Bitcoin testnet can be disabled)
        // network groups order should be the same as the order of all networks in `Filters`
        let assetGroupNumber = bitcoinTestnetEnabled ? 7 : 6
        XCTAssertEqual(lastUpdatedAssetGroups.count, assetGroupNumber)

        let offset = bitcoinTestnetEnabled ? 1 : 0
        if bitcoinTestnetEnabled {

          guard let btcTestnetGroup = lastUpdatedAssetGroups[safe: 4]
          else {
            XCTFail("Unexpected test result")
            return
          }
          XCTAssertEqual(btcTestnetGroup.groupType, .network(.mockBitcoinTestnet))
          XCTAssertEqual(btcTestnetGroup.assets.count, 1)
          // BTC testnet (value = $0)
          XCTAssertEqual(
            btcTestnetGroup.assets[safe: 0]?.token.symbol,
            BraveWallet.BlockchainToken.mockBTCToken.symbol
          )
          XCTAssertEqual(btcTestnetGroup.assets[safe: 0]?.quantity, String(format: "%.04f", 0))
        }

        guard let ethMainnetGroup = lastUpdatedAssetGroups[safe: 0],
          let solMainnetGroup = lastUpdatedAssetGroups[safe: 1],
          let filTestnetGroup = lastUpdatedAssetGroups[safe: 2],
          let filMainnetGroup = lastUpdatedAssetGroups[safe: 3],
          let btcMainnetGroup = lastUpdatedAssetGroups[safe: 4 + offset],
          let ethSepoliaGroup = lastUpdatedAssetGroups[safe: 5 + offset]
        else {
          XCTFail("Unexpected test result")
          return
        }

        XCTAssertEqual(ethMainnetGroup.groupType, .network(.mockMainnet))
        XCTAssertEqual(ethMainnetGroup.assets.count, 2)  // ETH Mainnet, USDC
        // ETH (value ~= $2741.7510399999996)
        XCTAssertEqual(
          ethMainnetGroup.assets[safe: 0]?.token.symbol,
          BraveWallet.BlockchainToken.previewToken.symbol
        )
        XCTAssertEqual(
          ethMainnetGroup.assets[safe: 0]?.quantity,
          String(format: "%.04f", self.mockETHBalanceAccount1)
        )
        // USDC (value = $0.04)
        XCTAssertEqual(
          ethMainnetGroup.assets[safe: 1]?.token.symbol,
          BraveWallet.BlockchainToken.mockUSDCToken.symbol
        )
        XCTAssertEqual(
          ethMainnetGroup.assets[safe: 1]?.quantity,
          String(format: "%.04f", self.mockUSDCBalanceAccount1 + self.mockUSDCBalanceAccount2)
        )

        XCTAssertEqual(solMainnetGroup.groupType, .network(.mockSolana))
        XCTAssertEqual(solMainnetGroup.assets.count, 1)  // SOL
        // SOL (value = $775.3)
        XCTAssertEqual(
          solMainnetGroup.assets[safe: 0]?.token.symbol,
          BraveWallet.BlockchainToken.mockSolToken.symbol
        )
        XCTAssertEqual(
          solMainnetGroup.assets[safe: 0]?.quantity,
          String(format: "%.04f", self.mockSOLBalance)
        )

        XCTAssertEqual(filTestnetGroup.groupType, .network(.mockFilecoinTestnet))
        XCTAssertEqual(filTestnetGroup.assets.count, 1)  // FIL on testnet
        // FIL (value = $400)
        XCTAssertEqual(
          filTestnetGroup.assets[safe: 0]?.token.symbol,
          BraveWallet.BlockchainToken.mockFilToken.symbol
        )
        XCTAssertEqual(
          filTestnetGroup.assets[safe: 0]?.quantity,
          String(format: "%.04f", self.mockFILBalanceTestnet)
        )

        XCTAssertEqual(filMainnetGroup.groupType, .network(.mockFilecoinMainnet))
        XCTAssertEqual(filMainnetGroup.assets.count, 1)  // FIL on mainnet
        // FIL (value = $4)
        XCTAssertEqual(
          filMainnetGroup.assets[safe: 0]?.token.symbol,
          BraveWallet.BlockchainToken.mockFilToken.symbol
        )
        XCTAssertEqual(
          filMainnetGroup.assets[safe: 0]?.quantity,
          String(format: "%.04f", self.mockFILBalanceAccount1)
        )

        XCTAssertEqual(btcMainnetGroup.groupType, .network(.mockBitcoinMainnet))
        XCTAssertEqual(btcMainnetGroup.assets.count, 1)
        // BTC mainnet (value = $0.0065726)
        XCTAssertEqual(
          btcMainnetGroup.assets[safe: 0]?.token.symbol,
          BraveWallet.BlockchainToken.mockBTCToken.symbol
        )
        XCTAssertEqual(btcMainnetGroup.assets[safe: 0]?.quantity, String(format: "%.04f", 0))

        XCTAssertEqual(ethSepoliaGroup.groupType, .network(.mockSepolia))
        XCTAssertEqual(ethSepoliaGroup.assets.count, 1)  // ETH Sepolia
        // ETH Sepolia (value = $0)
        XCTAssertEqual(
          ethSepoliaGroup.assets[safe: 0]?.token.symbol,
          BraveWallet.BlockchainToken.previewToken.symbol
        )
        XCTAssertEqual(ethSepoliaGroup.assets[safe: 0]?.quantity, String(format: "%.04f", 0))

        // Verify NFTs not used in Portfolio #7945
        let noAssetsAreNFTs = lastUpdatedAssetGroups.flatMap(\.assets).allSatisfy({
          !($0.token.isNft || $0.token.isErc721)
        })
        XCTAssertTrue(noAssetsAreNFTs)
      }
      .store(in: &cancellables)
    isLocked = false
    store.saveFilters(
      .init(
        groupBy: .networks,
        sortOrder: store.filters.sortOrder,
        isHidingSmallBalances: store.filters.isHidingSmallBalances,
        isHidingUnownedNFTs: store.filters.isHidingUnownedNFTs,
        isShowingNFTNetworkLogo: store.filters.isShowingNFTNetworkLogo,
        accounts: [
          ethAccount1, ethAccount2, solAccount1, solAccount2, filAccount1, filAccount2,
          filTestnetAccount, btcAccount1, btcAccount2, btcTestnetAccount,
        ].map {
          .init(isSelected: true, model: $0)
        },
        networks: MockJsonRpcService.allKnownNetworks.map {
          .init(isSelected: true, model: $0)
        }
      )
    )
    await fulfillment(of: [assetGroupsExpectation], timeout: 1)
    cancellables.removeAll()
    // test hiding a network  & hiding groups with small balances
    let networksExpectation = expectation(description: "update-networks")
    store.$assetGroups
      .dropFirst()
      .collect(2)
      .sink { assetGroups in
        defer { networksExpectation.fulfill() }
        XCTAssertEqual(assetGroups.count, 2)  // empty (no balance, price, history), populated
        guard let lastUpdatedAssetGroups = assetGroups.last else {
          XCTFail("Unexpected test result")
          return
        }
        // grouping by .network; 1 group for Solana network, 1 group for Filecoin mainnet, 1 group for Filecoin testnet
        let groupNumber = bitcoinTestnetEnabled ? 4 : 3
        XCTAssertEqual(lastUpdatedAssetGroups.count, groupNumber)
        guard let solMainnetGroup = lastUpdatedAssetGroups[safe: 0],
          let filTestnetGroup = lastUpdatedAssetGroups[safe: 1],
          let filMainnetGroup = lastUpdatedAssetGroups[safe: 2]
        else {
          XCTFail("Unexpected test result")
          return
        }
        if bitcoinTestnetEnabled {
          guard let btcTestnetGroup = lastUpdatedAssetGroups[safe: 3]
          else {
            XCTFail("Unexpected test result")
            return
          }

          XCTAssertEqual(btcTestnetGroup.groupType, .network(.mockBitcoinTestnet))
          XCTAssertEqual(btcTestnetGroup.assets.count, 1)  // BTC
          // BTC (value = $0.65726)
          XCTAssertEqual(
            btcTestnetGroup.assets[safe: 0]?.token.symbol,
            BraveWallet.BlockchainToken.mockBTCToken.symbol
          )
          XCTAssertEqual(
            btcTestnetGroup.assets[safe: 0]?.quantity,
            String(format: "%.04f", self.mockBTCBalanceTestnet)
          )
        }

        XCTAssertEqual(solMainnetGroup.groupType, .network(.mockSolana))
        XCTAssertEqual(solMainnetGroup.assets.count, 1)  // SOL
        // SOL (value = $775.3)
        XCTAssertEqual(
          solMainnetGroup.assets[safe: 0]?.token.symbol,
          BraveWallet.BlockchainToken.mockSolToken.symbol
        )
        XCTAssertEqual(
          solMainnetGroup.assets[safe: 0]?.quantity,
          String(format: "%.04f", self.mockSOLBalance)
        )

        XCTAssertEqual(filTestnetGroup.groupType, .network(.mockFilecoinTestnet))
        XCTAssertEqual(filTestnetGroup.assets.count, 1)  // FIL
        // FIL (value = $400)
        XCTAssertEqual(
          filTestnetGroup.assets[safe: 0]?.token.symbol,
          BraveWallet.BlockchainToken.mockFilToken.symbol
        )
        XCTAssertEqual(
          filTestnetGroup.assets[safe: 0]?.quantity,
          String(format: "%.04f", self.mockFILBalanceTestnet)
        )

        XCTAssertEqual(filMainnetGroup.groupType, .network(.mockFilecoinMainnet))
        XCTAssertEqual(filMainnetGroup.assets.count, 1)  // FIL
        // FIL (value = $4)
        XCTAssertEqual(
          filMainnetGroup.assets[safe: 0]?.token.symbol,
          BraveWallet.BlockchainToken.mockFilToken.symbol
        )
        XCTAssertEqual(
          filMainnetGroup.assets[safe: 0]?.quantity,
          String(format: "%.04f", self.mockFILBalanceAccount1)
        )
        // eth mainnet group hidden as network de-selected
        // sepolia network group hidden for small balance
        // Bitcoin network group hidden for small balance
        let lastIndex = bitcoinTestnetEnabled ? 4 : 3
        XCTAssertNil(lastUpdatedAssetGroups[safe: lastIndex])
      }
      .store(in: &cancellables)
    store.saveFilters(
      .init(
        groupBy: .networks,
        sortOrder: store.filters.sortOrder,
        isHidingSmallBalances: true,
        isHidingUnownedNFTs: store.filters.isHidingUnownedNFTs,
        isShowingNFTNetworkLogo: store.filters.isShowingNFTNetworkLogo,
        accounts: [
          ethAccount1, ethAccount2, solAccount1, solAccount2, filAccount1, filAccount2,
          filTestnetAccount, btcAccount1, btcAccount2, btcTestnetAccount,
        ].map {
          .init(isSelected: true, model: $0)
        },
        networks: MockJsonRpcService.allKnownNetworks.map {
          // hide ethNetwork
          .init(isSelected: $0 != .mockMainnet, model: $0)
        }
      )
    )
    await fulfillment(of: [networksExpectation], timeout: 1)
    cancellables.removeAll()
  }

  func testGroupByNetworks() async {
    await groupByNetworksHelper(bitcoinTestnetEnabled: false)
  }

  func testGroupByNetworkBitcoinTestnet() async {
    await groupByNetworksHelper(bitcoinTestnetEnabled: true)
  }
}

extension BraveWallet.BlockchainToken {
  /// Returns a copy of the `BlockchainToken` with the given `visible` flag and `isSpam` flag.
  func copy(asVisibleAsset isVisible: Bool, isSpam: Bool = false) -> Self {
    (self.copy() as! Self).then {
      $0.visible = isVisible
      $0.isSpam = isSpam
    }
  }
}
