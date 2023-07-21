// Copyright 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Combine
import XCTest
import BraveCore
import Preferences
@testable import BraveWallet

class PortfolioStoreTests: XCTestCase {

  private var cancellables: Set<AnyCancellable> = .init()
  
  override func setUp() {
    resetFilters()
  }
  override func tearDown() {
    resetFilters()
  }
  private func resetFilters() {
    Preferences.Wallet.sortOrderFilter.reset()
    Preferences.Wallet.isHidingSmallBalancesFilter.reset()
    Preferences.Wallet.nonSelectedAccountsFilter.reset()
    Preferences.Wallet.nonSelectedNetworksFilter.reset()
  }
  
  /// Test `update()` will fetch all visible user assets from all networks and display them sorted by their balance.
  func testUpdate() async {
    let mockETHBalance: Double = 0.896
    let mockETHPrice: String = "3059.99" // ETH value = $2741.75104
    let mockUSDCBalanceAccount1: Double = 0.5
    let mockUSDCBalanceAccount2: Double = 0.25
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
    let ethAccount1: BraveWallet.AccountInfo = .mockEthAccount
    let ethAccount2 = (BraveWallet.AccountInfo.mockEthAccount.copy() as! BraveWallet.AccountInfo).then {
      $0.address = "mock_eth_id_2"
      $0.name = "Ethereum Account 2"
    }
    let mockEthAccountInfos: [BraveWallet.AccountInfo] = [ethAccount1, ethAccount2]
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
    var totalUSDCBalanceValue: Double = 0
    totalUSDCBalanceValue += (Double(mockUSDCAssetPrice.price) ?? 0) * mockUSDCBalanceAccount1
    totalUSDCBalanceValue += (Double(mockUSDCAssetPrice.price) ?? 0) * mockUSDCBalanceAccount2
    
    let totalBalanceValue = totalEthBalanceValue + totalSolBalanceValue + totalUSDCBalanceValue
    let totalBalance = currencyFormatter.string(from: NSNumber(value: totalBalanceValue)) ?? ""
    
    let ethKeyring: BraveWallet.KeyringInfo = .init(
      id: BraveWallet.KeyringId.default,
      isKeyringCreated: true,
      isLocked: false,
      isBackedUp: true,
      accountInfos: mockEthAccountInfos
    )
    let solKeyring: BraveWallet.KeyringInfo = .init(
      id: BraveWallet.KeyringId.solana,
      isKeyringCreated: true,
      isLocked: false,
      isBackedUp: true,
      accountInfos: mockSolAccountInfos
    )
    
    // setup test services
    let keyringService = BraveWallet.TestKeyringService()
    keyringService._addObserver = { _ in }
    keyringService._isLocked = { completion in
      // unlocked would cause `update()` from call in `init` to be called prior to test being setup.g
      completion(true)
    }
    keyringService._allAccounts = {
      $0(.init(accounts: ethKeyring.accountInfos + solKeyring.accountInfos))
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
    rpcService._balance = { accountAddress, _, _, completion in
      // eth balance
      if accountAddress == ethAccount1.address {
        completion(ethBalanceWei, .success, "")
      } else {
        completion("", .success, "")
      }
    }
    rpcService._erc20TokenBalance = { contractAddress, accountAddress, _, completion in
      // usdc balance
      if accountAddress == ethAccount1.address {
        completion(usdcAccount1BalanceWei, .success, "")
      } else {
        completion(usdcAccount2BalanceWei, .success, "")
      }
    }
    rpcService._erc721TokenBalance = { contractAddress, _, accountAddress, _, completion in
      // eth nft balance
      if accountAddress == ethAccount1.address {
        completion(mockNFTBalanceWei, .success, "")
      } else {
        completion("", .success, "")
      }
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
    mockAssetManager._getAllVisibleAssetsInNetworkAssets = { networks in
      [
        NetworkAssets(network: .mockMainnet, tokens: mockEthUserAssets.filter({ $0.visible == true }), sortOrder: 0),
        NetworkAssets(network: .mockSolana, tokens: mockSolUserAssets.filter({ $0.visible == true }), sortOrder: 1)
      ].filter { networkAsset in networks.contains(where: { $0 == networkAsset.network }) }
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
    
    // MARK: Default update() Test
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
        // ETH (value ~= $2741.7510399999996)
        XCTAssertEqual(lastUpdatedVisibleAssets[safe: 0]?.token.symbol,
                       BraveWallet.BlockchainToken.previewToken.symbol)
        XCTAssertEqual(lastUpdatedVisibleAssets[safe: 0]?.price,
                       mockETHAssetPrice.price)
        XCTAssertEqual(lastUpdatedVisibleAssets[safe: 0]?.history,
                       mockETHPriceHistory)
        XCTAssertEqual(lastUpdatedVisibleAssets[safe: 0]?.decimalBalance,
                       mockETHBalance)
        // SOL (value = $775.3)
        XCTAssertEqual(lastUpdatedVisibleAssets[safe: 1]?.token.symbol,
                       BraveWallet.BlockchainToken.mockSolToken.symbol)
        XCTAssertEqual(lastUpdatedVisibleAssets[safe: 1]?.price,
                       mockSOLAssetPrice.price)
        XCTAssertEqual(lastUpdatedVisibleAssets[safe: 1]?.history,
                       mockSOLPriceHistory)
        XCTAssertEqual(lastUpdatedVisibleAssets[safe: 1]?.decimalBalance,
                       mockSOLBalance)
        // USDC (value $0.5)
        XCTAssertEqual(lastUpdatedVisibleAssets[safe: 2]?.token.symbol,
                       BraveWallet.BlockchainToken.mockUSDCToken.symbol)
        XCTAssertEqual(lastUpdatedVisibleAssets[safe: 2]?.price,
                       mockUSDCAssetPrice.price)
        XCTAssertEqual(lastUpdatedVisibleAssets[safe: 2]?.history,
                       mockUSDCPriceHistory)
        XCTAssertEqual(lastUpdatedVisibleAssets[safe: 2]?.decimalBalance,
                       mockUSDCBalanceAccount1 + mockUSDCBalanceAccount2)
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
    await fulfillment(of: [userVisibleAssetsException, balanceException, isLoadingBalancesException], timeout: 1)
    cancellables.removeAll()
    
    // MARK: Sort Order Filter Test (Smallest value first)
    let sortExpectation = expectation(description: "update-sortOrder")
    store.$userVisibleAssets
      .dropFirst()
      .collect(2)
      .sink { userVisibleAssets in
        defer { sortExpectation.fulfill() }
        XCTAssertEqual(userVisibleAssets.count, 2) // empty assets, populated assets
        guard let lastUpdatedVisibleAssets = userVisibleAssets.last else {
          XCTFail("Unexpected test result")
          return
        }
        // USDC on Ethereum mainnet, SOL on Solana mainnet, ETH on Ethereum mainnet
        XCTAssertEqual(lastUpdatedVisibleAssets.count, 3)
        // USDC (value $0.75)
        XCTAssertEqual(lastUpdatedVisibleAssets[safe: 0]?.token.symbol,
                       BraveWallet.BlockchainToken.mockUSDCToken.symbol)
        XCTAssertEqual(lastUpdatedVisibleAssets[safe: 0]?.price,
                       mockUSDCAssetPrice.price)
        XCTAssertEqual(lastUpdatedVisibleAssets[safe: 0]?.history,
                       mockUSDCPriceHistory)
        XCTAssertEqual(lastUpdatedVisibleAssets[safe: 0]?.decimalBalance,
                       mockUSDCBalanceAccount1 + mockUSDCBalanceAccount2)
        // SOL (value = $775.3)
        XCTAssertEqual(lastUpdatedVisibleAssets[safe: 1]?.token.symbol,
                       BraveWallet.BlockchainToken.mockSolToken.symbol)
        XCTAssertEqual(lastUpdatedVisibleAssets[safe: 1]?.price,
                       mockSOLAssetPrice.price)
        XCTAssertEqual(lastUpdatedVisibleAssets[safe: 1]?.history,
                       mockSOLPriceHistory)
        XCTAssertEqual(lastUpdatedVisibleAssets[safe: 1]?.decimalBalance,
                       mockSOLBalance)
        // ETH (value ~= $2741.7510399999996)
        XCTAssertEqual(lastUpdatedVisibleAssets[safe: 2]?.token.symbol,
                       BraveWallet.BlockchainToken.previewToken.symbol)
        XCTAssertEqual(lastUpdatedVisibleAssets[safe: 2]?.price,
                       mockETHAssetPrice.price)
        XCTAssertEqual(lastUpdatedVisibleAssets[safe: 2]?.history,
                       mockETHPriceHistory)
        XCTAssertEqual(lastUpdatedVisibleAssets[safe: 2]?.decimalBalance,
                       mockETHBalance)
      }.store(in: &cancellables)
    
    // change sort to ascending
    store.saveFilters(.init(
      groupBy: store.filters.groupBy,
      sortOrder: .valueAsc,
      isHidingSmallBalances: store.filters.isHidingSmallBalances,
      isHidingUnownedNFTs: store.filters.isHidingUnownedNFTs,
      isShowingNFTNetworkLogo: store.filters.isShowingNFTNetworkLogo,
      accounts: store.filters.accounts,
      networks: store.filters.networks
    ))
    await fulfillment(of: [sortExpectation], timeout: 1)
    cancellables.removeAll()
    
    // MARK: Hide Small Balances Test (tokens with value < $1 hidden)
    let hideSmallBalancesExpectation = expectation(description: "update-hideSmallBalances")
    store.$userVisibleAssets
      .dropFirst()
      .collect(2)
      .sink { userVisibleAssets in
        defer { hideSmallBalancesExpectation.fulfill() }
        XCTAssertEqual(userVisibleAssets.count, 2) // empty assets, populated assets
        guard let lastUpdatedVisibleAssets = userVisibleAssets.last else {
          XCTFail("Unexpected test result")
          return
        }
        // ETH on Ethereum mainnet, SOL on Solana mainnet
        XCTAssertEqual(lastUpdatedVisibleAssets.count, 2) // USDC hidden for small balance
        // ETH (value ~= 2741.7510399999996)
        XCTAssertEqual(lastUpdatedVisibleAssets[safe: 0]?.token.symbol,
                       BraveWallet.BlockchainToken.previewToken.symbol)
        XCTAssertEqual(lastUpdatedVisibleAssets[safe: 0]?.price,
                       mockETHAssetPrice.price)
        XCTAssertEqual(lastUpdatedVisibleAssets[safe: 0]?.history,
                       mockETHPriceHistory)
        XCTAssertEqual(lastUpdatedVisibleAssets[safe: 0]?.decimalBalance,
                       mockETHBalance)
        // SOL (value = 775.3)
        XCTAssertEqual(lastUpdatedVisibleAssets[safe: 1]?.token.symbol,
                       BraveWallet.BlockchainToken.mockSolToken.symbol)
        XCTAssertEqual(lastUpdatedVisibleAssets[safe: 1]?.price,
                       mockSOLAssetPrice.price)
        XCTAssertEqual(lastUpdatedVisibleAssets[safe: 1]?.history,
                       mockSOLPriceHistory)
        XCTAssertEqual(lastUpdatedVisibleAssets[safe: 1]?.decimalBalance,
                       mockSOLBalance)
        // USDC (value 0.75), hidden
        XCTAssertNil(lastUpdatedVisibleAssets[safe: 2])
      }.store(in: &cancellables)
    store.saveFilters(.init(
      groupBy: store.filters.groupBy,
      sortOrder: .valueDesc,
      isHidingSmallBalances: true,
      isHidingUnownedNFTs: store.filters.isHidingUnownedNFTs,
      isShowingNFTNetworkLogo: store.filters.isShowingNFTNetworkLogo,
      accounts: store.filters.accounts,
      networks: store.filters.networks
    ))
    await fulfillment(of: [hideSmallBalancesExpectation], timeout: 1)
    cancellables.removeAll()
    
    // MARK: Account Filter Test (Ethereum account 2 de-selected)
    let accountsExpectation = expectation(description: "update-accounts")
    store.$userVisibleAssets
      .dropFirst()
      .collect(2)
      .sink { userVisibleAssets in
        defer { accountsExpectation.fulfill() }
        XCTAssertEqual(userVisibleAssets.count, 2) // empty assets, populated assets
        guard let lastUpdatedVisibleAssets = userVisibleAssets.last else {
          XCTFail("Unexpected test result")
          return
        }
        // ETH on Ethereum mainnet, SOL on Solana mainnet, USDC on Ethereum mainnet
        XCTAssertEqual(lastUpdatedVisibleAssets.count, 3)
        // ETH (value ~= 2741.7510399999996)
        XCTAssertEqual(lastUpdatedVisibleAssets[safe: 0]?.token.symbol,
                       BraveWallet.BlockchainToken.previewToken.symbol)
        XCTAssertEqual(lastUpdatedVisibleAssets[safe: 0]?.price,
                       mockETHAssetPrice.price)
        XCTAssertEqual(lastUpdatedVisibleAssets[safe: 0]?.history,
                       mockETHPriceHistory)
        XCTAssertEqual(lastUpdatedVisibleAssets[safe: 0]?.decimalBalance,
                       mockETHBalance)
        // SOL (value = 775.3)
        XCTAssertEqual(lastUpdatedVisibleAssets[safe: 1]?.token.symbol,
                       BraveWallet.BlockchainToken.mockSolToken.symbol)
        XCTAssertEqual(lastUpdatedVisibleAssets[safe: 1]?.price,
                       mockSOLAssetPrice.price)
        XCTAssertEqual(lastUpdatedVisibleAssets[safe: 1]?.history,
                       mockSOLPriceHistory)
        XCTAssertEqual(lastUpdatedVisibleAssets[safe: 1]?.decimalBalance,
                       mockSOLBalance)
        // USDC (value 0.5, ethAccount2 hidden!)
        XCTAssertEqual(lastUpdatedVisibleAssets[safe: 2]?.token.symbol,
                       BraveWallet.BlockchainToken.mockUSDCToken.symbol)
        XCTAssertEqual(lastUpdatedVisibleAssets[safe: 2]?.price,
                       mockUSDCAssetPrice.price)
        XCTAssertEqual(lastUpdatedVisibleAssets[safe: 2]?.history,
                       mockUSDCPriceHistory)
        XCTAssertEqual(lastUpdatedVisibleAssets[safe: 2]?.decimalBalance,
                       mockUSDCBalanceAccount1) // verify account 2 hidden
      }.store(in: &cancellables)
    store.saveFilters(.init(
      groupBy: store.filters.groupBy,
      sortOrder: .valueDesc,
      isHidingSmallBalances: false,
      isHidingUnownedNFTs: store.filters.isHidingUnownedNFTs,
      isShowingNFTNetworkLogo: store.filters.isShowingNFTNetworkLogo,
      accounts: store.filters.accounts.map { // deselect ethAccount2
        .init(isSelected: $0.model.address != ethAccount2.address, model: $0.model)
      },
      networks: store.filters.networks
    ))
    await fulfillment(of: [accountsExpectation], timeout: 1)
    cancellables.removeAll()
    
    // MARK: Network Filter Test (Solana networks de-selected)
    let networksExpectation = expectation(description: "update-networks")
    store.$userVisibleAssets
      .dropFirst()
      .collect(2)
      .sink { userVisibleAssets in
        defer { networksExpectation.fulfill() }
        XCTAssertEqual(userVisibleAssets.count, 2) // empty assets, populated assets
        guard let lastUpdatedVisibleAssets = userVisibleAssets.last else {
          XCTFail("Unexpected test result")
          return
        }
        // ETH on Ethereum mainnet, USDC on Ethereum mainnet
        XCTAssertEqual(lastUpdatedVisibleAssets.count, 2)
        // ETH (value ~= 2741.7510399999996)
        XCTAssertEqual(lastUpdatedVisibleAssets[safe: 0]?.token.symbol,
                       BraveWallet.BlockchainToken.previewToken.symbol)
        XCTAssertEqual(lastUpdatedVisibleAssets[safe: 0]?.price,
                       mockETHAssetPrice.price)
        XCTAssertEqual(lastUpdatedVisibleAssets[safe: 0]?.history,
                       mockETHPriceHistory)
        XCTAssertEqual(lastUpdatedVisibleAssets[safe: 0]?.decimalBalance,
                       mockETHBalance)
        // USDC (value 0.75)
        XCTAssertEqual(lastUpdatedVisibleAssets[safe: 1]?.token.symbol,
                       BraveWallet.BlockchainToken.mockUSDCToken.symbol)
        XCTAssertEqual(lastUpdatedVisibleAssets[safe: 1]?.price,
                       mockUSDCAssetPrice.price)
        XCTAssertEqual(lastUpdatedVisibleAssets[safe: 1]?.history,
                       mockUSDCPriceHistory)
        XCTAssertEqual(lastUpdatedVisibleAssets[safe: 1]?.decimalBalance,
                       mockUSDCBalanceAccount1 + mockUSDCBalanceAccount2)
        // SOL (value = 0, SOL networks hidden)
        XCTAssertNil(lastUpdatedVisibleAssets[safe: 2])
      }.store(in: &cancellables)
    store.saveFilters(.init(
      groupBy: store.filters.groupBy,
      sortOrder: .valueDesc,
      isHidingSmallBalances: false,
      isHidingUnownedNFTs: store.filters.isHidingUnownedNFTs,
      isShowingNFTNetworkLogo: store.filters.isShowingNFTNetworkLogo,
      accounts: store.filters.accounts.map { // re-select all accounts
        .init(isSelected: true, model: $0.model)
      },
      networks: store.filters.networks.map { // only select Ethereum networks
        .init(isSelected: $0.model.coin == .eth, model: $0.model)
      }
    ))
    await fulfillment(of: [networksExpectation], timeout: 1)
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
