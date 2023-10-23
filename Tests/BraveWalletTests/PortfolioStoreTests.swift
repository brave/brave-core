// Copyright 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Combine
import XCTest
import BraveCore
import Preferences
@testable import BraveWallet

@MainActor class PortfolioStoreTests: XCTestCase {

  private var cancellables: Set<AnyCancellable> = .init()
  private let currencyFormatter = NumberFormatter().then { $0.numberStyle = .currency }
  
  override func setUp() {
    resetFilters()
  }
  override func tearDown() {
    resetFilters()
  }
  private func resetFilters() {
    Preferences.Wallet.showTestNetworks.reset()
    Preferences.Wallet.groupByFilter.reset()
    Preferences.Wallet.sortOrderFilter.reset()
    Preferences.Wallet.isHidingSmallBalancesFilter.reset()
    Preferences.Wallet.nonSelectedAccountsFilter.reset()
    Preferences.Wallet.nonSelectedNetworksFilter.reset()
  }
  
  // Accounts
  let ethAccount1: BraveWallet.AccountInfo = .mockEthAccount
  let ethAccount2 = (BraveWallet.AccountInfo.mockEthAccount.copy() as! BraveWallet.AccountInfo).then {
    $0.address = "mock_eth_id_2"
    $0.name = "Ethereum Account 2"
  }
  let solAccount1: BraveWallet.AccountInfo = .mockSolAccount
  let solAccount2 = (BraveWallet.AccountInfo.mockSolAccount.copy() as! BraveWallet.AccountInfo).then {
    $0.address = "mock_sol_id_2"
    $0.name = "Solana Account 2"
  }
  let filAccount1: BraveWallet.AccountInfo = .mockFilAccount
  let filAccount2 = (BraveWallet.AccountInfo.mockFilAccount.copy() as! BraveWallet.AccountInfo).then {
    $0.address = "mock_fil_id_2"
    $0.name = "Filecoin Account 2"
  }
  let filTestnetAccount: BraveWallet.AccountInfo = .mockFilTestnetAccount

  // Networks
  let ethNetwork: BraveWallet.NetworkInfo = .mockMainnet
  let goerliNetwork: BraveWallet.NetworkInfo = .mockGoerli
  let solNetwork: BraveWallet.NetworkInfo = .mockSolana
  let filMainnet: BraveWallet.NetworkInfo = .mockFilecoinMainnet
  let filTestnet: BraveWallet.NetworkInfo = .mockFilecoinTestnet
  // ETH Asset, balance, price, history
  let mockETHBalanceAccount1: Double = 0.896
  let mockETHPrice: String = "3059.99" // ETH value = $2741.75104
  lazy var mockETHAssetPrice: BraveWallet.AssetPrice = .init(
    fromAsset: "eth", toAsset: "usd",
    price: mockETHPrice, assetTimeframeChange: "-57.23"
  )
  lazy var mockETHPriceHistory: [BraveWallet.AssetTimePrice] = [
    .init(date: Date(timeIntervalSinceNow: -1000), price: "3000.00"),
    .init(date: Date(), price: mockETHPrice)
  ]
  // USDC Asset, balance, price, history
  let mockUSDCBalanceAccount1: Double = 0.03
  let mockUSDCBalanceAccount2: Double = 0.01
  let mockUSDCPrice: String = "1" // USDC total value = $0.04
  lazy var mockUSDCAssetPrice: BraveWallet.AssetPrice = .init(
    fromAsset: BraveWallet.BlockchainToken.mockUSDCToken.assetRatioId,
    toAsset: "usd", price: mockUSDCPrice, assetTimeframeChange: "-57.23"
  )
  lazy var mockUSDCPriceHistory: [BraveWallet.AssetTimePrice] = [
    .init(date: Date(timeIntervalSinceNow: -1000), price: "0.999"),
    .init(date: Date(), price: mockUSDCPrice)
  ]
  // SOL Asset, balance, price, history
  let mockSOLBalance: Double = 3.8765 // lamports rounded
  let mockSOLPrice: String = "200" // SOL value = $775.30
  lazy var mockSOLAssetPrice: BraveWallet.AssetPrice = .init(
    fromAsset: "sol", toAsset: "usd",
    price: mockSOLPrice, assetTimeframeChange: "-57.23"
  )
  lazy var mockSOLPriceHistory: [BraveWallet.AssetTimePrice] = [
    .init(date: Date(timeIntervalSinceNow: -1000), price: mockSOLPrice),
    .init(date: Date(), price: "250.00")
  ]
  
  // FIL Asset, balance, price, history on filecoin mainnet
  let mockFILBalanceAccount1: Double = 1
  let mockFILPrice: String = "4.00" // FIL value on mainnet = $4.00
  lazy var mockFILAssetPrice: BraveWallet.AssetPrice = .init(
    fromAsset: "fil", toAsset: "usd",
    price: mockFILPrice, assetTimeframeChange: "-57.23"
  )
  lazy var mockFILPriceHistory: [BraveWallet.AssetTimePrice] = [
    .init(date: Date(timeIntervalSinceNow: -1000), price: "4.06"),
    .init(date: Date(), price: mockFILPrice)
  ]
  // FIL Asset, balance on filecoin testnet
  let mockFILBalanceTestnet: Double = 100 // FIL value on testnet = $400.00
  
  var totalBalance: String {
    let totalEthBalanceValue: Double = (Double(mockETHAssetPrice.price) ?? 0) * mockETHBalanceAccount1
    var totalUSDCBalanceValue: Double = 0
    totalUSDCBalanceValue += (Double(mockUSDCAssetPrice.price) ?? 0) * mockUSDCBalanceAccount1
    totalUSDCBalanceValue += (Double(mockUSDCAssetPrice.price) ?? 0) * mockUSDCBalanceAccount2
    let totalSolBalanceValue: Double = (Double(mockSOLAssetPrice.price) ?? 0) * mockSOLBalance
    let totalFilBalanceValue: Double = (Double(mockFILAssetPrice.price) ?? 0) * mockFILBalanceAccount1
    let totalBalanceValue = totalEthBalanceValue + totalSolBalanceValue + totalUSDCBalanceValue + totalFilBalanceValue
    return currencyFormatter.string(from: NSNumber(value: totalBalanceValue)) ?? ""
  }
  
  private func setupStore() -> PortfolioStore {
    let mockSOLLamportBalance: UInt64 = 3876535000 // ~3.8765 SOL
    let formatter = WeiFormatter(decimalFormatStyle: .decimals(precision: 18))
    
    // config filecoin on mainnet
    let mockFilUserAssets: [BraveWallet.BlockchainToken] = [
      BraveWallet.NetworkInfo.mockFilecoinMainnet.nativeToken.copy(asVisibleAsset: true)
    ]
    let mockFilBalanceInWei = formatter.weiString(
      from: mockFILBalanceAccount1,
      radix: .decimal,
      decimals: Int(BraveWallet.BlockchainToken.mockFilToken.decimals)
    ) ?? ""
    // config filecoin on testnet
    let mockFilTestnetBalanceInWei = formatter.weiString(
      from: mockFILBalanceTestnet,
      radix: .decimal,
      decimals: Int(BraveWallet.BlockchainToken.mockFilToken.decimals)
    ) ?? ""
    
    // config Solana
    let mockSolUserAssets: [BraveWallet.BlockchainToken] = [
      BraveWallet.NetworkInfo.mockSolana.nativeToken.copy(asVisibleAsset: true),
      .mockSpdToken // Verify non-visible assets not displayed #6386
    ]
    // config Ethereum
    let mockEthUserAssets: [BraveWallet.BlockchainToken] = [
      .previewToken.copy(asVisibleAsset: true),
      .previewDaiToken, // Verify non-visible assets not displayed #6386
      .mockUSDCToken.copy(asVisibleAsset: true),
      .mockERC721NFTToken.copy(asVisibleAsset: true) // Verify NFTs not used in Portfolio #7945
    ]
    let ethBalanceWei = formatter.weiString(
      from: mockETHBalanceAccount1,
      radix: .hex,
      decimals: Int(BraveWallet.BlockchainToken.previewToken.decimals)
    ) ?? ""
    let usdcAccount1BalanceWei = formatter.weiString(
      from: mockUSDCBalanceAccount1,
      radix: .hex,
      decimals: Int(BraveWallet.BlockchainToken.mockUSDCToken.decimals)
    ) ?? ""
    let usdcAccount2BalanceWei = formatter.weiString(
      from: mockUSDCBalanceAccount2,
      radix: .hex,
      decimals: Int(BraveWallet.BlockchainToken.mockUSDCToken.decimals)
    ) ?? ""
    
    let mockEthGoerliUserAssets: [BraveWallet.BlockchainToken] = [
      goerliNetwork.nativeToken.copy(asVisibleAsset: true)
    ]
    
    let mockFilTestnetUserAssets: [BraveWallet.BlockchainToken] = [
      filTestnet.nativeToken.copy(asVisibleAsset: true)
    ]
    
    // setup test services
    let keyringService = BraveWallet.TestKeyringService()
    keyringService._addObserver = { _ in }
    keyringService._isLocked = { completion in
      // unlocked would cause `update()` from call in `init` to be called prior to test being setup.g
      completion(true)
    }
    keyringService._allAccounts = {
      $0(.init(
        accounts: [
          self.ethAccount1, self.ethAccount2,
          self.solAccount1, self.solAccount2,
          self.filAccount1, self.filAccount2,
          self.filTestnetAccount
        ],
        selectedAccount: self.ethAccount1,
        ethDappSelectedAccount: self.ethAccount1,
        solDappSelectedAccount: self.solAccount1
      ))
    }
    let rpcService = BraveWallet.TestJsonRpcService()
    rpcService._addObserver = { _ in }
    rpcService._allNetworks = { coin, completion in
      switch coin {
      case .eth:
        completion([self.ethNetwork, self.goerliNetwork])
      case .sol:
        completion([self.solNetwork])
      case .fil:
        completion([self.filMainnet, self.filTestnet])
      case .btc:
        XCTFail("Should not fetch bitcoin network")
      @unknown default:
        XCTFail("Should not fetch unknown network")
      }
    }
    rpcService._balance = { accountAddress, coin, chainId, completion in
      // eth balance
      if coin == .eth {
        if chainId == self.ethNetwork.chainId, accountAddress == self.ethAccount1.address {
          completion(ethBalanceWei, .success, "")
        } else {
          completion("", .success, "")
        }
      } else { // .fil
        if chainId == self.filMainnet.chainId {
          if accountAddress == self.filAccount1.address {
            completion(mockFilBalanceInWei, .success, "")
          } else {
            completion("", .success, "")
          }
        } else {
          completion(mockFilTestnetBalanceInWei, .success, "")
        }
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
    rpcService._erc721TokenBalance = { _, _, _, _, completion in
      // should not be fetching NFT balance in Portfolio
      completion("", .internalError, "Error Message")
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
      completion(true, [self.mockETHAssetPrice, self.mockUSDCAssetPrice, self.mockSOLAssetPrice, self.mockFILAssetPrice])
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
        completion(true, self.mockFILPriceHistory) // for both mainnet and testnet
      default:
        completion(false, [])
      }
    }
    
    let mockAssetManager = TestableWalletUserAssetManager()
    mockAssetManager._getAllUserAssetsInNetworkAssetsByVisibility = { networks, _ in
      [
        NetworkAssets(network: .mockMainnet, tokens: mockEthUserAssets.filter(\.visible), sortOrder: 0),
        NetworkAssets(network: .mockSolana, tokens: mockSolUserAssets.filter(\.visible), sortOrder: 1),
        NetworkAssets(network: .mockGoerli, tokens: mockEthGoerliUserAssets.filter(\.visible), sortOrder: 2),
        NetworkAssets(network: .mockFilecoinMainnet, tokens: mockFilUserAssets.filter(\.visible), sortOrder: 3),
        NetworkAssets(network: .mockFilecoinTestnet, tokens: mockFilTestnetUserAssets.filter(\.visible), sortOrder: 4)
      ].filter { networkAsset in networks.contains(where: { $0 == networkAsset.network }) }
    }
    return PortfolioStore(
      keyringService: keyringService,
      rpcService: rpcService,
      walletService: walletService,
      assetRatioService: assetRatioService,
      blockchainRegistry: BraveWallet.TestBlockchainRegistry(),
      ipfsApi: TestIpfsAPI(),
      userAssetManager: mockAssetManager
    )
  }
  
  /// Test `update()` will fetch all visible user assets from all networks and display them sorted by their balance.
  func testUpdate() async {
    Preferences.Wallet.showTestNetworks.value = false
    let store = setupStore()
    
    // MARK: Default update() Test
    let assetGroupsExpectation = expectation(description: "update-assetGroups")
    XCTAssertTrue(store.assetGroups.isEmpty)  // Initial state
    store.$assetGroups
      .dropFirst()
      .collect(2)
      .sink { assetGroups in
        defer { assetGroupsExpectation.fulfill() }
        XCTAssertEqual(assetGroups.count, 2) // empty (no balance, price, history), populated
        
        guard let lastUpdatedAssetGroups = assetGroups.last else {
          XCTFail("Unexpected test result")
          return
        }
        XCTAssertEqual(lastUpdatedAssetGroups.count, 1) // grouping by .none, so only 1 group
        guard let group = lastUpdatedAssetGroups.first else {
          XCTFail("Unexpected test result")
          return
        }
        
        // ETH on Ethereum mainnet, SOL on Solana mainnet, USDC on Ethereum mainnet, ETH on Goerli, FIL on Filecoin mainnet, FIL on Filecoin testnet
        XCTAssertEqual(group.assets.count, 4)
        // ETH Mainnet (value ~= $2741.7510399999996)
        XCTAssertEqual(group.assets[safe: 0]?.token.symbol,
                       BraveWallet.BlockchainToken.previewToken.symbol)
        XCTAssertEqual(group.assets[safe: 0]?.price,
                       self.mockETHAssetPrice.price)
        XCTAssertEqual(group.assets[safe: 0]?.history,
                       self.mockETHPriceHistory)
        XCTAssertEqual(group.assets[safe: 0]?.quantity,
                       String(format: "%.04f", self.mockETHBalanceAccount1))
        
        // SOL (value = $775.3)
        XCTAssertEqual(group.assets[safe: 1]?.token.symbol,
                       BraveWallet.BlockchainToken.mockSolToken.symbol)
        XCTAssertEqual(group.assets[safe: 1]?.price,
                       self.mockSOLAssetPrice.price)
        XCTAssertEqual(group.assets[safe: 1]?.history,
                       self.mockSOLPriceHistory)
        XCTAssertEqual(group.assets[safe: 1]?.quantity,
                       String(format: "%.04f", self.mockSOLBalance))
        
        // FIL (value $4.00) on mainnet
        XCTAssertEqual(group.assets[safe: 2]?.token.symbol,
                       BraveWallet.BlockchainToken.mockFilToken.symbol)
        XCTAssertEqual(group.assets[safe: 2]?.price,
                       self.mockFILAssetPrice.price)
        XCTAssertEqual(group.assets[safe: 2]?.history,
                       self.mockFILPriceHistory)
        XCTAssertEqual(group.assets[safe: 2]?.quantity,
                       String(format: "%.04f", self.mockFILBalanceAccount1))
        
        // USDC (value $0.04)
        XCTAssertEqual(group.assets[safe: 3]?.token.symbol,
                       BraveWallet.BlockchainToken.mockUSDCToken.symbol)
        XCTAssertEqual(group.assets[safe: 3]?.price,
                       self.mockUSDCAssetPrice.price)
        XCTAssertEqual(group.assets[safe: 3]?.history,
                       self.mockUSDCPriceHistory)
        XCTAssertEqual(group.assets[safe: 3]?.quantity,
                       String(format: "%.04f", self.mockUSDCBalanceAccount1 + self.mockUSDCBalanceAccount2))
        
        // ETH Goerli (value = 0), hidden because test networks not selected by default
        // FIL Testnet (value = $400.00), hidden because test networks not selected by default
        XCTAssertNil(group.assets[safe: 4])
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
    store.update()
    await fulfillment(of: [assetGroupsExpectation, balanceExpectation, isLoadingBalancesExpectation], timeout: 1)
    cancellables.removeAll()
  }
  
  /// Test `assetGroups` will be sorted to from smallest to highest fiat value when `sortOrder` filter is `valueAsc`.
  func testFilterSort() async {
    Preferences.Wallet.showTestNetworks.value = true
    let store = setupStore()
    let sortExpectation = expectation(description: "update-sortOrder")
    store.$assetGroups
      .dropFirst()
      .collect(2)
      .sink { assetGroups in
        defer { sortExpectation.fulfill() }
        XCTAssertEqual(assetGroups.count, 2) // empty (no balance, price, history), populated
        guard let lastUpdatedAssetGroups = assetGroups.last else {
          XCTFail("Unexpected test result")
          return
        }
        XCTAssertEqual(lastUpdatedAssetGroups.count, 1) // grouping by .none, so only 1 group
        guard let group = lastUpdatedAssetGroups.first else {
          XCTFail("Unexpected test result")
          return
        }
        // USDC on Ethereum mainnet, SOL on Solana mainnet, ETH on Ethereum mainnet, FIL on Filecoin mainnet and FIL on Filecoin testnet
        XCTAssertEqual(group.assets.count, 6)
        // ETH Goerli (value = $0)
        XCTAssertEqual(group.assets[safe: 0]?.token.symbol,
                       BraveWallet.BlockchainToken.previewToken.symbol)
        XCTAssertEqual(group.assets[safe: 0]?.quantity, String(format: "%.04f", 0))
        // USDC (value = $0.04)
        XCTAssertEqual(group.assets[safe: 1]?.token.symbol,
                       BraveWallet.BlockchainToken.mockUSDCToken.symbol)
        XCTAssertEqual(group.assets[safe: 1]?.quantity,
                       String(format: "%.04f", self.mockUSDCBalanceAccount1 + self.mockUSDCBalanceAccount2))
        // FIL (value = $4.00) on filecoin mainnet
        XCTAssertEqual(group.assets[safe: 2]?.token.symbol,
                       BraveWallet.BlockchainToken.mockFilToken.symbol)
        XCTAssertEqual(group.assets[safe: 2]?.quantity,
                       String(format: "%.04f", self.mockFILBalanceAccount1))
        // FIL (value = $400.00) on filecoin testnet
        XCTAssertEqual(group.assets[safe: 3]?.token.symbol,
                       BraveWallet.BlockchainToken.mockFilToken.symbol)
        XCTAssertEqual(group.assets[safe: 3]?.quantity,
                       String(format: "%.04f", self.mockFILBalanceTestnet))
        // SOL (value = $775.3)
        XCTAssertEqual(group.assets[safe: 4]?.token.symbol,
                       BraveWallet.BlockchainToken.mockSolToken.symbol)
        XCTAssertEqual(group.assets[safe: 4]?.quantity,
                       String(format: "%.04f", self.mockSOLBalance))
        // ETH Mainnet (value ~= $2741.7510399999996)
        XCTAssertEqual(group.assets[safe: 5]?.token.symbol,
                       BraveWallet.BlockchainToken.previewToken.symbol)
        XCTAssertEqual(group.assets[safe: 5]?.quantity,
                       String(format: "%.04f", self.mockETHBalanceAccount1))
      }.store(in: &cancellables)

    // change sort to ascending
    store.saveFilters(.init(
      groupBy: store.filters.groupBy,
      sortOrder: .valueAsc,
      isHidingSmallBalances: store.filters.isHidingSmallBalances,
      isHidingUnownedNFTs: store.filters.isHidingUnownedNFTs,
      isShowingNFTNetworkLogo: store.filters.isShowingNFTNetworkLogo,
      accounts: [ethAccount1, ethAccount2, solAccount1, solAccount2, filAccount1, filAccount2, filTestnetAccount].map {
        .init(isSelected: true, model: $0)
      },
      networks: [ethNetwork, goerliNetwork, solNetwork, filMainnet, filTestnet].map {
        .init(isSelected: true, model: $0)
      }
    ))
    await fulfillment(of: [sortExpectation], timeout: 1)
    cancellables.removeAll()
  }
  
  /// Test `assetGroups` will be filtered to remove small balances when `hideSmallBalances` filter is true.
  func testHideSmallBalances() async {
    Preferences.Wallet.showTestNetworks.value = true
    let store = setupStore()
    let hideSmallBalancesExpectation = expectation(description: "update-hideSmallBalances")
    store.$assetGroups
      .dropFirst()
      .collect(2)
      .sink { assetGroups in
        defer { hideSmallBalancesExpectation.fulfill() }
        XCTAssertEqual(assetGroups.count, 2) // empty (no balance, price, history), populated
        guard let lastUpdatedAssetGroups = assetGroups.last else {
          XCTFail("Unexpected test result")
          return
        }
        XCTAssertEqual(lastUpdatedAssetGroups.count, 1) // grouping by .none, so only 1 group
        guard let group = lastUpdatedAssetGroups.first else {
          XCTFail("Unexpected test result")
          return
        }
        // ETH on Ethereum mainnet, SOL on Solana mainnet, FIL on Filecoin mainnet and testnet
        XCTAssertEqual(group.assets.count, 4) // USDC, ETH Goerli hidden for small balance
        // ETH Mainnet (value ~= $2741.7510399999996)
        XCTAssertEqual(group.assets[safe: 0]?.token.symbol,
                       BraveWallet.BlockchainToken.previewToken.symbol)
        XCTAssertEqual(group.assets[safe: 0]?.quantity,
                       String(format: "%.04f", self.mockETHBalanceAccount1))
        // SOL (value = $775.3)
        XCTAssertEqual(group.assets[safe: 1]?.token.symbol,
                       BraveWallet.BlockchainToken.mockSolToken.symbol)
        XCTAssertEqual(group.assets[safe: 1]?.quantity,
                       String(format: "%.04f", self.mockSOLBalance))
        // FIL (value = $400) on Filecoin testnet
        XCTAssertEqual(group.assets[safe: 2]?.token.symbol,
                       BraveWallet.BlockchainToken.mockFilToken.symbol)
        XCTAssertEqual(group.assets[safe: 2]?.quantity,
                       String(format: "%.04f", self.mockFILBalanceTestnet))
        // FIL (value = $4) on Filecoin mainnet
        XCTAssertEqual(group.assets[safe: 3]?.token.symbol,
                       BraveWallet.BlockchainToken.mockFilToken.symbol)
        XCTAssertEqual(group.assets[safe: 3]?.quantity,
                       String(format: "%.04f", self.mockFILBalanceAccount1))
        // USDC (value = $0.04), hidden
        XCTAssertNil(group.assets[safe: 4])
      }.store(in: &cancellables)
    store.saveFilters(.init(
      groupBy: store.filters.groupBy,
      sortOrder: .valueDesc,
      isHidingSmallBalances: true,
      isHidingUnownedNFTs: store.filters.isHidingUnownedNFTs,
      isShowingNFTNetworkLogo: store.filters.isShowingNFTNetworkLogo,
      accounts: [ethAccount1, ethAccount2, solAccount1, solAccount2, filAccount1, filAccount2, filTestnetAccount].map {
        .init(isSelected: true, model: $0)
      },
      networks: [ethNetwork, goerliNetwork, solNetwork, filMainnet, filTestnet].map {
        .init(isSelected: true, model: $0)
      }
    ))
    await fulfillment(of: [hideSmallBalancesExpectation], timeout: 1)
    cancellables.removeAll()
  }
  
  /// Test `assetGroups` will be filtered by accounts when `accounts` filter is has de-selected accounts.
  func testFilterAccounts() async {
    Preferences.Wallet.showTestNetworks.value = true
    let store = setupStore()
    let accountsExpectation = expectation(description: "update-accounts")
    store.$assetGroups
      .dropFirst()
      .collect(2)
      .sink { assetGroups in
        defer { accountsExpectation.fulfill() }
        XCTAssertEqual(assetGroups.count, 2) // empty (no balance, price, history), populated
        guard let lastUpdatedAssetGroups = assetGroups.last else {
          XCTFail("Unexpected test result")
          return
        }
        XCTAssertEqual(lastUpdatedAssetGroups.count, 1) // grouping by .none, so only 1 group
        guard let group = lastUpdatedAssetGroups.first else {
          XCTFail("Unexpected test result")
          return
        }
        // ETH on Ethereum mainnet, SOL on Solana mainnet, USDC on Ethereum mainnet, ETH on Goerli, FIL on mainnet and testnet
        XCTAssertEqual(group.assets.count, 6)
        // ETH Mainnet (value ~= $2741.7510399999996)
        XCTAssertEqual(group.assets[safe: 0]?.token.symbol,
                       BraveWallet.BlockchainToken.previewToken.symbol)
        XCTAssertEqual(group.assets[safe: 0]?.quantity,
                       String(format: "%.04f", self.mockETHBalanceAccount1))
        // SOL (value = $775.3)
        XCTAssertEqual(group.assets[safe: 1]?.token.symbol,
                       BraveWallet.BlockchainToken.mockSolToken.symbol)
        XCTAssertEqual(group.assets[safe: 1]?.quantity,
                       String(format: "%.04f", self.mockSOLBalance))
        // FIL (value = $400) on testnet
        XCTAssertEqual(group.assets[safe: 2]?.token.symbol,
                       BraveWallet.BlockchainToken.mockFilToken.symbol)
        XCTAssertEqual(group.assets[safe: 2]?.quantity,
                       String(format: "%.04f", self.mockFILBalanceTestnet))
        // FIL (value = $4) on mainnet
        XCTAssertEqual(group.assets[safe: 3]?.token.symbol,
                       BraveWallet.BlockchainToken.mockFilToken.symbol)
        XCTAssertEqual(group.assets[safe: 3]?.quantity,
                       String(format: "%.04f", self.mockFILBalanceAccount1))
        // USDC (value = $0.03, ethAccount2 hidden!)
        XCTAssertEqual(group.assets[safe: 4]?.token.symbol,
                       BraveWallet.BlockchainToken.mockUSDCToken.symbol)
        XCTAssertEqual(group.assets[safe: 4]?.quantity,
                       String(format: "%.04f", self.mockUSDCBalanceAccount1)) // verify account 2 hidden
        // ETH Goerli (value = $0)
        XCTAssertEqual(group.assets[safe: 5]?.token.symbol,
                       self.goerliNetwork.nativeToken.symbol)
        XCTAssertEqual(group.assets[safe: 5]?.quantity, String(format: "%.04f", 0))
      }.store(in: &cancellables)
    store.saveFilters(.init(
      groupBy: store.filters.groupBy,
      sortOrder: .valueDesc,
      isHidingSmallBalances: false,
      isHidingUnownedNFTs: store.filters.isHidingUnownedNFTs,
      isShowingNFTNetworkLogo: store.filters.isShowingNFTNetworkLogo,
      accounts: [ethAccount1, ethAccount2, solAccount1, solAccount2, filAccount1, filAccount2, filTestnetAccount].map { // deselect ethAccount2
        .init(isSelected: $0 != ethAccount2, model: $0)
      },
      networks: [ethNetwork, goerliNetwork, solNetwork, filMainnet, filTestnet].map {
        .init(isSelected: true, model: $0)
      }
    ))
    await fulfillment(of: [accountsExpectation], timeout: 1)
    cancellables.removeAll()
  }
  
  /// Test `assetGroups` will be filtered by network when `networks` filter is has de-selected networks.
  func testFilterNetworks() async {
    Preferences.Wallet.showTestNetworks.value = true
    let store = setupStore()
    let networksExpectation = expectation(description: "update-networks")
    store.$assetGroups
      .dropFirst()
      .collect(2)
      .sink { assetGroups in
        defer { networksExpectation.fulfill() }
        XCTAssertEqual(assetGroups.count, 2) // empty (no balance, price, history), populated
        guard let lastUpdatedAssetGroups = assetGroups.last else {
          XCTFail("Unexpected test result")
          return
        }
        XCTAssertEqual(lastUpdatedAssetGroups.count, 1) // grouping by .none, so only 1 group
        guard let group = lastUpdatedAssetGroups.first else {
          XCTFail("Unexpected test result")
          return
        }
        // ETH on Ethereum mainnet, USDC on Ethereum mainnet, ETH on Goerli, FIL on mainnet and testnet
        XCTAssertEqual(group.assets.count, 5)
        // ETH Mainnet (value ~= $2741.7510399999996)
        XCTAssertEqual(group.assets[safe: 0]?.token.symbol,
                       BraveWallet.BlockchainToken.previewToken.symbol)
        XCTAssertEqual(group.assets[safe: 0]?.quantity,
                       String(format: "%.04f", self.mockETHBalanceAccount1))
        // FIL (value = $400) on testnet
        XCTAssertEqual(group.assets[safe: 1]?.token.symbol,
                       BraveWallet.BlockchainToken.mockFilToken.symbol)
        XCTAssertEqual(group.assets[safe: 1]?.quantity,
                       String(format: "%.04f", self.mockFILBalanceTestnet))
        // FIL (value = $4) on mainnet
        XCTAssertEqual(group.assets[safe: 2]?.token.symbol,
                       BraveWallet.BlockchainToken.mockFilToken.symbol)
        XCTAssertEqual(group.assets[safe: 2]?.quantity,
                       String(format: "%.04f", self.mockFILBalanceAccount1))
        // USDC (value = $0.04)
        XCTAssertEqual(group.assets[safe: 3]?.token.symbol,
                       BraveWallet.BlockchainToken.mockUSDCToken.symbol)
        XCTAssertEqual(group.assets[safe: 3]?.quantity,
                       String(format: "%.04f", self.mockUSDCBalanceAccount1 + self.mockUSDCBalanceAccount2))
        // ETH Goerli (value = $0)
        XCTAssertEqual(group.assets[safe: 4]?.token.symbol,
                       self.goerliNetwork.nativeToken.symbol)
        XCTAssertEqual(group.assets[safe: 4]?.quantity, String(format: "%.04f", 0))
        // SOL (value = $0, SOL networks hidden)
        XCTAssertNil(group.assets[safe: 5])
        
        // Verify NFTs not used in Portfolio #7945
        let noAssetsAreNFTs = lastUpdatedAssetGroups.flatMap(\.assets).allSatisfy({
          !($0.token.isNft || $0.token.isErc721)
        })
        XCTAssertTrue(noAssetsAreNFTs)
      }.store(in: &cancellables)
    store.saveFilters(.init(
      groupBy: store.filters.groupBy,
      sortOrder: .valueDesc,
      isHidingSmallBalances: false,
      isHidingUnownedNFTs: store.filters.isHidingUnownedNFTs,
      isShowingNFTNetworkLogo: store.filters.isShowingNFTNetworkLogo,
      accounts: [ethAccount1, ethAccount2, solAccount1, solAccount2, filAccount1, filAccount2, filTestnetAccount].map {
        .init(isSelected: true, model: $0)
      },
      networks: [ethNetwork, goerliNetwork, solNetwork, filMainnet, filTestnet].map { // only select Ethereum networks
        .init(isSelected: $0.coin == .eth || $0.coin == .fil, model: $0)
      }
    ))
    await fulfillment(of: [networksExpectation], timeout: 1)
  }
  
  /// Test `assetGroups` will be grouped by account when `GroupBy` filter is assigned `.account`.
  /// Additionally, test de-selecting/hiding one of the available accounts.
  func testGroupByAccounts() async {
    Preferences.Wallet.showTestNetworks.value = true
    let store = setupStore()
    let assetGroupsExpectation = expectation(description: "update-assetGroups")
    XCTAssertTrue(store.assetGroups.isEmpty)  // Initial state
    store.$assetGroups
      .dropFirst()
      .collect(2)
      .sink { assetGroups in
        defer { assetGroupsExpectation.fulfill() }
        XCTAssertEqual(assetGroups.count, 2) // empty (no balance, price, history), populated
        guard let lastUpdatedAssetGroups = assetGroups.last else {
          XCTFail("Unexpected test result")
          return
        }
        // grouping by .account; 1 for each of the 4 accounts
        XCTAssertEqual(lastUpdatedAssetGroups.count, 7)
        guard let ethAccount1Group = lastUpdatedAssetGroups[safe: 0],
              let solAccount1Group = lastUpdatedAssetGroups[safe: 1],
              let filTestnetAccountGroup = lastUpdatedAssetGroups[safe: 2],
              let filAccount1Group = lastUpdatedAssetGroups[safe: 3],
              let ethAccount2Group = lastUpdatedAssetGroups[safe: 4],
              let solAccount2Group = lastUpdatedAssetGroups[safe: 5],
              let filAccount2Group = lastUpdatedAssetGroups[safe: 6] else {
          XCTFail("Unexpected test result")
          return
        }
        XCTAssertEqual(ethAccount1Group.groupType, .account(self.ethAccount1))
        XCTAssertEqual(ethAccount1Group.assets.count, 3) // ETH Mainnet, USDC, ETH Goerli
        // ETH Mainnet (value ~= $2741.7510399999996)
        XCTAssertEqual(ethAccount1Group.assets[safe: 0]?.token.symbol,
                       BraveWallet.BlockchainToken.previewToken.symbol)
        XCTAssertEqual(ethAccount1Group.assets[safe: 0]?.quantity,
                       String(format: "%.04f", self.mockETHBalanceAccount1))
        // USDC (value = $0.03)
        XCTAssertEqual(ethAccount1Group.assets[safe: 1]?.token.symbol,
                       BraveWallet.BlockchainToken.mockUSDCToken.symbol)
        XCTAssertEqual(ethAccount1Group.assets[safe: 1]?.quantity,
                       String(format: "%.04f", self.mockUSDCBalanceAccount1))
        // ETH Goerli (value = $0)
        XCTAssertEqual(ethAccount1Group.assets[safe: 2]?.token.symbol,
                       self.goerliNetwork.nativeToken.symbol)
        XCTAssertEqual(ethAccount1Group.assets[safe: 2]?.quantity, String(format: "%.04f", 0))
        
        XCTAssertEqual(solAccount1Group.groupType, .account(self.solAccount1))
        XCTAssertEqual(solAccount1Group.assets.count, 1)
        // SOL (value = $775.3)
        XCTAssertEqual(solAccount1Group.assets[safe: 0]?.token.symbol,
                       BraveWallet.BlockchainToken.mockSolToken.symbol)
        XCTAssertEqual(solAccount1Group.assets[safe: 0]?.quantity,
                       String(format: "%.04f", self.mockSOLBalance))
        
        XCTAssertEqual(filTestnetAccountGroup.groupType, .account(self.filTestnetAccount))
        XCTAssertEqual(filTestnetAccountGroup.assets.count, 1)
        // FIL (value = $400)
        XCTAssertEqual(filTestnetAccountGroup.assets[safe: 0]?.token.symbol,
                       BraveWallet.BlockchainToken.mockFilToken.symbol)
        XCTAssertEqual(filTestnetAccountGroup.assets[safe: 0]?.quantity,
                       String(format: "%.04f", self.mockFILBalanceTestnet))
        
        XCTAssertEqual(filAccount1Group.groupType, .account(self.filAccount1))
        XCTAssertEqual(filAccount1Group.assets.count, 1)
        // FIL (value = $4)
        XCTAssertEqual(filAccount1Group.assets[safe: 0]?.token.symbol,
                       BraveWallet.BlockchainToken.mockFilToken.symbol)
        XCTAssertEqual(filAccount1Group.assets[safe: 0]?.quantity,
                       String(format: "%.04f", self.mockFILBalanceAccount1))
        
        XCTAssertEqual(ethAccount2Group.groupType, .account(self.ethAccount2))
        XCTAssertEqual(ethAccount2Group.assets.count, 3) // ETH Mainnet, USDC, ETH Goerli
        // USDC (value $0.01)
        XCTAssertEqual(ethAccount2Group.assets[safe: 0]?.token.symbol,
                       BraveWallet.BlockchainToken.mockUSDCToken.symbol)
        XCTAssertEqual(ethAccount2Group.assets[safe: 0]?.quantity,
                       String(format: "%.04f", self.mockUSDCBalanceAccount2))
        // ETH Mainnet (value = $0)
        XCTAssertEqual(ethAccount2Group.assets[safe: 1]?.token.symbol,
                       BraveWallet.BlockchainToken.previewToken.symbol)
        XCTAssertEqual(ethAccount2Group.assets[safe: 1]?.quantity, String(format: "%.04f", 0))
        // ETH Goerli (value = $0)
        XCTAssertEqual(ethAccount2Group.assets[safe: 2]?.token.symbol,
                       self.goerliNetwork.nativeToken.symbol)
        XCTAssertEqual(ethAccount2Group.assets[safe: 2]?.quantity, String(format: "%.04f", 0))
        
        XCTAssertEqual(solAccount2Group.groupType, .account(self.solAccount2))
        XCTAssertEqual(solAccount2Group.assets.count, 1)
        // SOL (value = $0)
        XCTAssertEqual(solAccount2Group.assets[safe: 0]?.token.symbol,
                       BraveWallet.BlockchainToken.mockSolToken.symbol)
        XCTAssertEqual(solAccount2Group.assets[safe: 0]?.quantity, String(format: "%.04f", 0))
        
        XCTAssertEqual(filAccount2Group.groupType, .account(self.filAccount2))
        XCTAssertEqual(filAccount2Group.assets.count, 1)
        // FIL (value = $0)
        XCTAssertEqual(filAccount2Group.assets[safe: 0]?.token.symbol,
                       BraveWallet.BlockchainToken.mockFilToken.symbol)
        XCTAssertEqual(filAccount2Group.assets[safe: 0]?.quantity, String(format: "%.04f", 0))
        
        // Verify NFTs not used in Portfolio #7945
        let noAssetsAreNFTs = lastUpdatedAssetGroups.flatMap(\.assets).allSatisfy({
          !($0.token.isNft || $0.token.isErc721)
        })
        XCTAssertTrue(noAssetsAreNFTs)
      }
      .store(in: &cancellables)
    store.saveFilters(.init(
      groupBy: .accounts,
      sortOrder: store.filters.sortOrder,
      isHidingSmallBalances: store.filters.isHidingSmallBalances,
      isHidingUnownedNFTs: store.filters.isHidingUnownedNFTs,
      isShowingNFTNetworkLogo: store.filters.isShowingNFTNetworkLogo,
      accounts: [ethAccount1, ethAccount2, solAccount1, solAccount2, filAccount1, filAccount2, filTestnetAccount].map {
        .init(isSelected: true, model: $0)
      },
      networks: [ethNetwork, goerliNetwork, solNetwork, filMainnet, filTestnet].map {
        .init(isSelected: true, model: $0)
      }
    ))
    await fulfillment(of: [assetGroupsExpectation], timeout: 1)
    cancellables.removeAll()
    // test hiding an account & hiding groups with small balances
    let accountsExpectation = expectation(description: "update-accounts")
    store.$assetGroups
      .dropFirst()
      .collect(2)
      .sink { assetGroups in
        defer { accountsExpectation.fulfill() }
        XCTAssertEqual(assetGroups.count, 2) // empty (no balance, price, history), populated
        guard let lastUpdatedAssetGroups = assetGroups.last else {
          XCTFail("Unexpected test result")
          return
        }
        // grouping by .account; 1 for each of the 2 accounts selected accounts
        XCTAssertEqual(lastUpdatedAssetGroups.count, 4)
        guard let ethAccount1Group = lastUpdatedAssetGroups[safe: 0],
              let solAccountGroup = lastUpdatedAssetGroups[safe: 1],
              let filTestnetAccountGroup = lastUpdatedAssetGroups[safe: 2],
              let filAccount1Group = lastUpdatedAssetGroups[safe: 3] else {
          XCTFail("Unexpected test result")
          return
        }
        XCTAssertEqual(ethAccount1Group.groupType, .account(self.ethAccount1))
        XCTAssertEqual(ethAccount1Group.assets.count, 1) // ETH Mainnet (USDC, ETH Goerli hidden for small balance)
        // ETH Mainnet (value ~= $2741.7510399999996)
        XCTAssertEqual(ethAccount1Group.assets[safe: 0]?.token.symbol,
                       BraveWallet.BlockchainToken.previewToken.symbol)
        XCTAssertEqual(ethAccount1Group.assets[safe: 0]?.quantity,
                       String(format: "%.04f", self.mockETHBalanceAccount1))
        // USDC (value $0.03)
        XCTAssertNil(ethAccount1Group.assets[safe: 1])
        
        XCTAssertEqual(solAccountGroup.groupType, .account(.mockSolAccount))
        XCTAssertEqual(solAccountGroup.assets.count, 1)
        // SOL (value = $775.3)
        XCTAssertEqual(solAccountGroup.assets[safe: 0]?.token.symbol,
                       BraveWallet.BlockchainToken.mockSolToken.symbol)
        XCTAssertEqual(solAccountGroup.assets[safe: 0]?.quantity,
                       String(format: "%.04f", self.mockSOLBalance))
        
        XCTAssertEqual(filTestnetAccountGroup.groupType, .account(self.filTestnetAccount))
        XCTAssertEqual(filTestnetAccountGroup.assets.count, 1)
        // FIL (value = $400)
        XCTAssertEqual(filTestnetAccountGroup.assets[safe: 0]?.token.symbol,
                       BraveWallet.BlockchainToken.mockFilToken.symbol)
        XCTAssertEqual(filTestnetAccountGroup.assets[safe: 0]?.quantity,
                       String(format: "%.04f", self.mockFILBalanceTestnet))
        
        XCTAssertEqual(filAccount1Group.groupType, .account(self.filAccount1))
        XCTAssertEqual(filAccount1Group.assets.count, 1)
        // FIL (value = $4)
        XCTAssertEqual(filAccount1Group.assets[safe: 0]?.token.symbol,
                       BraveWallet.BlockchainToken.mockFilToken.symbol)
        XCTAssertEqual(filAccount1Group.assets[safe: 0]?.quantity,
                       String(format: "%.04f", self.mockFILBalanceAccount1))
        
        // ethAccount2 hidden as it's de-selected, solAccount2 hidden for small balance, filAccount2 hidden for small balance
        XCTAssertNil(lastUpdatedAssetGroups[safe: 4])
        XCTAssertNil(lastUpdatedAssetGroups[safe: 5])
        XCTAssertNil(lastUpdatedAssetGroups[safe: 6])
      }
      .store(in: &cancellables)
    store.saveFilters(.init(
      groupBy: .accounts,
      sortOrder: store.filters.sortOrder,
      isHidingSmallBalances: true,
      isHidingUnownedNFTs: store.filters.isHidingUnownedNFTs,
      isShowingNFTNetworkLogo: store.filters.isShowingNFTNetworkLogo,
      accounts: [ethAccount1, ethAccount2, solAccount1, solAccount2, filAccount1, filAccount2, filTestnetAccount].map {
        .init(isSelected: $0 != ethAccount2, model: $0)
      },
      networks: [ethNetwork, goerliNetwork, solNetwork, filMainnet, filTestnet].map {
        .init(isSelected: true, model: $0)
      }
    ))
    await fulfillment(of: [accountsExpectation], timeout: 1)
  }
  
  /// Test `assetGroups` will be grouped by network when `GroupBy` filter is assigned `.network`.
  /// Additionally, test de-selecting/hiding one of the available networks.
  func testGroupByNetworks() async {
    Preferences.Wallet.showTestNetworks.value = true
    let store = setupStore()
    let assetGroupsExpectation = expectation(description: "update-assetGroups")
    XCTAssertTrue(store.assetGroups.isEmpty)  // Initial state
    store.$assetGroups
      .dropFirst()
      .collect(2)
      .sink { assetGroups in
        defer { assetGroupsExpectation.fulfill() }
        XCTAssertEqual(assetGroups.count, 2) // empty (no balance, price, history), populated
        guard let lastUpdatedAssetGroups = assetGroups.last else {
          XCTFail("Unexpected test result")
          return
        }
        // grouping by .network; 1 for each of the 2 networks
        XCTAssertEqual(lastUpdatedAssetGroups.count, 5)
        guard let ethMainnetGroup = lastUpdatedAssetGroups[safe: 0],
              let solMainnetGroup = lastUpdatedAssetGroups[safe: 1],
              let filTestnetGroup = lastUpdatedAssetGroups[safe: 2],
              let filMainnetGroup = lastUpdatedAssetGroups[safe: 3],
              let ethGoerliGroup = lastUpdatedAssetGroups[safe: 4] else {
          XCTFail("Unexpected test result")
          return
        }
        XCTAssertEqual(ethMainnetGroup.groupType, .network(.mockMainnet))
        XCTAssertEqual(ethMainnetGroup.assets.count, 2) // ETH Mainnet, USDC
        // ETH (value ~= $2741.7510399999996)
        XCTAssertEqual(ethMainnetGroup.assets[safe: 0]?.token.symbol,
                       BraveWallet.BlockchainToken.previewToken.symbol)
        XCTAssertEqual(ethMainnetGroup.assets[safe: 0]?.quantity,
                       String(format: "%.04f", self.mockETHBalanceAccount1))
        // USDC (value = $0.04)
        XCTAssertEqual(ethMainnetGroup.assets[safe: 1]?.token.symbol,
                       BraveWallet.BlockchainToken.mockUSDCToken.symbol)
        XCTAssertEqual(ethMainnetGroup.assets[safe: 1]?.quantity,
                       String(format: "%.04f", self.mockUSDCBalanceAccount1 + self.mockUSDCBalanceAccount2))
        
        XCTAssertEqual(solMainnetGroup.groupType, .network(.mockSolana))
        XCTAssertEqual(solMainnetGroup.assets.count, 1) // SOL
        // SOL (value = $775.3)
        XCTAssertEqual(solMainnetGroup.assets[safe: 0]?.token.symbol,
                       BraveWallet.BlockchainToken.mockSolToken.symbol)
        XCTAssertEqual(solMainnetGroup.assets[safe: 0]?.quantity,
                       String(format: "%.04f", self.mockSOLBalance))
        
        XCTAssertEqual(filTestnetGroup.groupType, .network(.mockFilecoinTestnet))
        XCTAssertEqual(filTestnetGroup.assets.count, 1) // FIL on testnet
        // FIL (value = $400)
        XCTAssertEqual(filTestnetGroup.assets[safe: 0]?.token.symbol,
                       BraveWallet.BlockchainToken.mockFilToken.symbol)
        XCTAssertEqual(filTestnetGroup.assets[safe: 0]?.quantity,
                       String(format: "%.04f", self.mockFILBalanceTestnet))
        
        XCTAssertEqual(filMainnetGroup.groupType, .network(.mockFilecoinMainnet))
        XCTAssertEqual(filMainnetGroup.assets.count, 1) // FIL on mainnet
        // FIL (value = $4)
        XCTAssertEqual(filMainnetGroup.assets[safe: 0]?.token.symbol,
                       BraveWallet.BlockchainToken.mockFilToken.symbol)
        XCTAssertEqual(filMainnetGroup.assets[safe: 0]?.quantity,
                       String(format: "%.04f", self.mockFILBalanceAccount1))
        
        XCTAssertEqual(ethGoerliGroup.groupType, .network(.mockGoerli))
        XCTAssertEqual(ethGoerliGroup.assets.count, 1) // ETH Goerli
        // ETH Goerli (value = $0)
        XCTAssertEqual(ethGoerliGroup.assets[safe: 0]?.token.symbol,
                       BraveWallet.BlockchainToken.previewToken.symbol)
        XCTAssertEqual(ethGoerliGroup.assets[safe: 0]?.quantity, String(format: "%.04f", 0))
        
        // Verify NFTs not used in Portfolio #7945
        let noAssetsAreNFTs = lastUpdatedAssetGroups.flatMap(\.assets).allSatisfy({
          !($0.token.isNft || $0.token.isErc721)
        })
        XCTAssertTrue(noAssetsAreNFTs)
      }
      .store(in: &cancellables)
    store.saveFilters(.init(
      groupBy: .networks,
      sortOrder: store.filters.sortOrder,
      isHidingSmallBalances: store.filters.isHidingSmallBalances,
      isHidingUnownedNFTs: store.filters.isHidingUnownedNFTs,
      isShowingNFTNetworkLogo: store.filters.isShowingNFTNetworkLogo,
      accounts: [ethAccount1, ethAccount2, solAccount1, solAccount2, filAccount1, filAccount2, filTestnetAccount].map {
        .init(isSelected: true, model: $0)
      },
      networks: [ethNetwork, goerliNetwork, solNetwork, filMainnet, filTestnet].map {
        .init(isSelected: true, model: $0)
      }
    ))
    await fulfillment(of: [assetGroupsExpectation], timeout: 1)
    cancellables.removeAll()
    // test hiding a network  & hiding groups with small balances
    let networksExpectation = expectation(description: "update-networks")
    store.$assetGroups
      .dropFirst()
      .collect(2)
      .sink { assetGroups in
        defer { networksExpectation.fulfill() }
        XCTAssertEqual(assetGroups.count, 2) // empty (no balance, price, history), populated
        guard let lastUpdatedAssetGroups = assetGroups.last else {
          XCTFail("Unexpected test result")
          return
        }
        // grouping by .network; 1 group for Solana network, 1 group for Filecoin mainnet, 1 group for Filecoin testnet
        XCTAssertEqual(lastUpdatedAssetGroups.count, 3)
        guard let solMainnetGroup = lastUpdatedAssetGroups[safe: 0],
              let filTestnetGroup = lastUpdatedAssetGroups[safe: 1],
              let filMainnetGroup = lastUpdatedAssetGroups[safe: 2]  else {
          XCTFail("Unexpected test result")
          return
        }
        XCTAssertEqual(solMainnetGroup.groupType, .network(.mockSolana))
        XCTAssertEqual(solMainnetGroup.assets.count, 1) // SOL
        // SOL (value = $775.3)
        XCTAssertEqual(solMainnetGroup.assets[safe: 0]?.token.symbol,
                       BraveWallet.BlockchainToken.mockSolToken.symbol)
        XCTAssertEqual(solMainnetGroup.assets[safe: 0]?.quantity,
                       String(format: "%.04f", self.mockSOLBalance))
        
        XCTAssertEqual(filTestnetGroup.groupType, .network(.mockFilecoinTestnet))
        XCTAssertEqual(filTestnetGroup.assets.count, 1) // FIL
        // FIL (value = $400)
        XCTAssertEqual(filTestnetGroup.assets[safe: 0]?.token.symbol,
                       BraveWallet.BlockchainToken.mockFilToken.symbol)
        XCTAssertEqual(filTestnetGroup.assets[safe: 0]?.quantity,
                       String(format: "%.04f", self.mockFILBalanceTestnet))
        
        XCTAssertEqual(filMainnetGroup.groupType, .network(.mockFilecoinMainnet))
        XCTAssertEqual(filMainnetGroup.assets.count, 1) // FIL
        // FIL (value = $4)
        XCTAssertEqual(filMainnetGroup.assets[safe: 0]?.token.symbol,
                       BraveWallet.BlockchainToken.mockFilToken.symbol)
        XCTAssertEqual(filMainnetGroup.assets[safe: 0]?.quantity,
                       String(format: "%.04f", self.mockFILBalanceAccount1))
        // eth mainnet group hidden as network de-selected
        XCTAssertNil(lastUpdatedAssetGroups[safe: 3])
        // goerli network group hidden for small balance
        XCTAssertNil(lastUpdatedAssetGroups[safe: 4])
      }
      .store(in: &cancellables)
    store.saveFilters(.init(
      groupBy: .networks,
      sortOrder: store.filters.sortOrder,
      isHidingSmallBalances: true,
      isHidingUnownedNFTs: store.filters.isHidingUnownedNFTs,
      isShowingNFTNetworkLogo: store.filters.isShowingNFTNetworkLogo,
      accounts: [ethAccount1, ethAccount2, solAccount1, solAccount2, filAccount1, filAccount2, filTestnetAccount].map {
        .init(isSelected: true, model: $0)
      },
      networks: [ethNetwork, goerliNetwork, solNetwork, filMainnet, filTestnet].map { // hide ethNetwork
        .init(isSelected: $0 != ethNetwork, model: $0)
      }
    ))
    await fulfillment(of: [networksExpectation], timeout: 1)
    cancellables.removeAll()
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
