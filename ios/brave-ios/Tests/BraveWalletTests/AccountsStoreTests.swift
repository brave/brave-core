// Copyright 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveCore
import Combine
import Preferences
import XCTest

@testable import BraveWallet

@MainActor class AccountsStoreTests: XCTestCase {

  override class func tearDown() {
    super.tearDown()
    Preferences.Wallet.isBitcoinTestnetEnabled.reset()
    Preferences.Wallet.isZcashTestnetEnabled.reset()
  }

  private var cancellables: Set<AnyCancellable> = .init()

  let ethAccount1: BraveWallet.AccountInfo = .mockEthAccount
  let ethAccount2 = (BraveWallet.AccountInfo.mockEthAccount.copy() as! BraveWallet.AccountInfo).then
  {
    $0.address = "mock_eth_id_2"
    $0.accountId.uniqueKey = $0.address
    $0.name = "Ethereum Account 2"
  }
  let solAccount1: BraveWallet.AccountInfo = .mockSolAccount
  let filAccount1: BraveWallet.AccountInfo = .mockFilAccount
  let filTestnetAccount: BraveWallet.AccountInfo = .mockFilTestnetAccount
  let btcAccount1: BraveWallet.AccountInfo = .mockBtcAccount
  let btcTestnetAccount: BraveWallet.AccountInfo = .mockBtcTestnetAccount
  let zcashAccount1: BraveWallet.AccountInfo = .mockZcashAccount
  let zcashTestnetAccount: BraveWallet.AccountInfo = .mockZcashTestnetAccount

  let mockETHBalanceAccount1: Double = 0.896
  let mockETHPrice: String = "3059.99"
  lazy var mockETHAssetPrice: BraveWallet.AssetPrice = .init(
    coin: .eth,
    chainId: BraveWallet.MainnetChainId,
    address: "",
    price: mockETHPrice,
    vsCurrency: "usd",
    cacheStatus: .hit,
    source: .coingecko,
    percentageChange24h: "-57.23"
  )
  let mockUSDCBalanceAccount1: Double = 0.03
  let mockUSDCBalanceAccount2: Double = 0.01
  let mockUSDCPrice: String = "1"
  lazy var mockUSDCAssetPrice: BraveWallet.AssetPrice = .init(
    coin: .eth,
    chainId: BraveWallet.MainnetChainId,
    address: BraveWallet.BlockchainToken.mockUSDCToken.contractAddress,
    price: mockUSDCPrice,
    vsCurrency: "usd",
    cacheStatus: .hit,
    source: .coingecko,
    percentageChange24h: "-57.23"
  )
  let ethMainnetTokens: [BraveWallet.BlockchainToken] = [
    BraveWallet.NetworkInfo.mockMainnet.nativeToken,
    .mockUSDCToken.copy(asVisibleAsset: true),
  ]

  let mockSOLBalance: Double = 3.8765
  let mockSOLPrice: String = "200"
  lazy var mockSOLAssetPrice: BraveWallet.AssetPrice = .init(
    coin: .sol,
    chainId: BraveWallet.SolanaMainnet,
    address: "",
    price: mockSOLPrice,
    vsCurrency: "usd",
    cacheStatus: .hit,
    source: .coingecko,
    percentageChange24h: "-57.23"
  )
  let solMainnetTokens: [BraveWallet.BlockchainToken] = [
    BraveWallet.NetworkInfo.mockSolana.nativeToken
  ]

  let mockFILBalanceAccount1: Double = 1
  let mockFILTestnetBalanceAccount1: Double = 10
  let mockFILPrice: String = "4.00"
  lazy var mockFILAssetPrice: BraveWallet.AssetPrice = .init(
    coin: .fil,
    chainId: BraveWallet.FilecoinMainnet,
    address: "",
    price: mockFILPrice,
    vsCurrency: "usd",
    cacheStatus: .hit,
    source: .coingecko,
    percentageChange24h: "-57.23"
  )
  lazy var mockFILTestnetAssetPrice: BraveWallet.AssetPrice = .init(
    coin: .fil,
    chainId: BraveWallet.FilecoinTestnet,
    address: "",
    price: mockFILPrice,
    vsCurrency: "usd",
    cacheStatus: .hit,
    source: .coingecko,
    percentageChange24h: "-57.23"
  )
  let filMainnetTokens: [BraveWallet.BlockchainToken] = [
    BraveWallet.NetworkInfo.mockFilecoinMainnet.nativeToken
  ]
  let filTestnetTokens: [BraveWallet.BlockchainToken] = [
    BraveWallet.NetworkInfo.mockFilecoinTestnet.nativeToken
  ]

  let mockBTCBalanceAccount1: Double = 0
  let mockBTCTestnetBalanceAccount1: Double = 0
  let mockBTCPrice: String = "65726.00"
  lazy var mockBTCAssetPrice: BraveWallet.AssetPrice = .init(
    coin: .btc,
    chainId: BraveWallet.BitcoinMainnet,
    address: "",
    price: mockBTCPrice,
    vsCurrency: "usd",
    cacheStatus: .hit,
    source: .coingecko,
    percentageChange24h: "-57.23"
  )
  lazy var mockBTCTestnetAssetPrice: BraveWallet.AssetPrice = .init(
    coin: .btc,
    chainId: BraveWallet.BitcoinTestnet,
    address: "",
    price: mockBTCPrice,
    vsCurrency: "usd",
    cacheStatus: .hit,
    source: .coingecko,
    percentageChange24h: "-57.23"
  )
  let mockZECBalanceAccount1: Double = 0
  let mockZECTestnetBalanceAccount1: Double = 0
  let mockZECPrice: String = "36.46"
  lazy var mockZECAssetPrice: BraveWallet.AssetPrice = .init(
    coin: .zec,
    chainId: BraveWallet.ZCashMainnet,
    address: "",
    price: mockZECPrice,
    vsCurrency: "usd",
    cacheStatus: .hit,
    source: .coingecko,
    percentageChange24h: "4.32"
  )
  lazy var mockZECTestnetAssetPrice: BraveWallet.AssetPrice = .init(
    coin: .zec,
    chainId: BraveWallet.ZCashTestnet,
    address: "",
    price: mockZECPrice,
    vsCurrency: "usd",
    cacheStatus: .hit,
    source: .coingecko,
    percentageChange24h: "4.32"
  )
  let btcMainnetTokens: [BraveWallet.BlockchainToken] = [
    BraveWallet.NetworkInfo.mockBitcoinMainnet.nativeToken
  ]
  let btcTestnetTokens: [BraveWallet.BlockchainToken] = [
    BraveWallet.NetworkInfo.mockBitcoinTestnet.nativeToken
  ]
  let zcashMainnetTokens: [BraveWallet.BlockchainToken] = [
    BraveWallet.NetworkInfo.mockZcashMainnet.nativeToken
  ]
  let zcashTestnetTokens: [BraveWallet.BlockchainToken] = [
    BraveWallet.NetworkInfo.mockZcashTestnet.nativeToken
  ]

  let formatter = WalletAmountFormatter(decimalFormatStyle: .decimals(precision: 18))

  func updateHelper(bitcoinAndZcashTestnetEnabled: Bool) async {
    Preferences.Wallet.isBitcoinTestnetEnabled.value = bitcoinAndZcashTestnetEnabled
    Preferences.Wallet.isZcashTestnetEnabled.value = bitcoinAndZcashTestnetEnabled
    let ethBalanceWei =
      formatter.weiString(
        from: mockETHBalanceAccount1,
        radix: .hex,
        decimals: Int(BraveWallet.NetworkInfo.mockMainnet.nativeToken.decimals)
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
    let mockSOLLamportBalance: UInt64 = 3_876_535_000  // ~3.8765 SOL
    let mockFilBalanceInWei =
      formatter.weiString(
        from: mockFILBalanceAccount1,
        radix: .decimal,
        decimals: Int(BraveWallet.NetworkInfo.mockFilecoinMainnet.nativeToken.decimals)
      ) ?? ""
    let mockFilTestnetBalanceInWei =
      formatter.weiString(
        from: mockFILBalanceAccount1,
        radix: .decimal,
        decimals: Int(BraveWallet.NetworkInfo.mockFilecoinTestnet.nativeToken.decimals)
      ) ?? ""
    let mockBtcBalanceInWei =
      formatter.weiString(
        from: mockBTCBalanceAccount1,
        radix: .decimal,
        decimals: Int(BraveWallet.NetworkInfo.mockBitcoinMainnet.nativeToken.decimals)
      ) ?? ""
    let mockBtcTestnetBalanceInWei =
      formatter.weiString(
        from: mockBTCTestnetBalanceAccount1,
        radix: .decimal,
        decimals: Int(BraveWallet.NetworkInfo.mockBitcoinTestnet.nativeToken.decimals)
      ) ?? ""
    let mockZcashBalanceInWei =
      formatter.weiString(
        from: mockZECBalanceAccount1,
        radix: .decimal,
        decimals: Int(BraveWallet.NetworkInfo.mockZcashMainnet.nativeToken.decimals)
      ) ?? ""
    let mockZcashTestnetBalanceInWei =
      formatter.weiString(
        from: mockZECTestnetBalanceAccount1,
        radix: .decimal,
        decimals: Int(BraveWallet.NetworkInfo.mockZcashTestnet.nativeToken.decimals)
      ) ?? ""

    let keyringService = BraveWallet.TestKeyringService()
    keyringService._addObserver = { _ in }
    keyringService._allAccounts = {
      $0(
        .init(
          accounts: [
            self.ethAccount1, self.ethAccount2,
            self.solAccount1, self.filAccount1,
            self.filTestnetAccount,
            self.btcAccount1, self.btcTestnetAccount,
            self.zcashAccount1, self.zcashTestnetAccount,
          ],
          selectedAccount: self.ethAccount1,
          ethDappSelectedAccount: self.ethAccount1,
          solDappSelectedAccount: self.solAccount1,
          adaDappSelectedAccount: nil
        )
      )
    }
    let rpcService = MockJsonRpcService()
    rpcService.hiddenNetworks.removeAll()
    rpcService._balance = { accountAddress, coin, chainId, completion in
      if coin == .eth,
        chainId == BraveWallet.MainnetChainId,
        accountAddress == self.ethAccount1.address
      {
        completion(ethBalanceWei, .success, "")
      } else if coin == .fil,
        chainId == BraveWallet.FilecoinMainnet,
        accountAddress == self.filAccount1.address
      {  // .fil
        completion(mockFilBalanceInWei, .success, "")
      } else if coin == .fil,
        chainId == BraveWallet.FilecoinTestnet,
        accountAddress == self.filTestnetAccount.address
      {
        completion(mockFilTestnetBalanceInWei, .success, "")
      } else if coin == .btc,
        chainId == BraveWallet.BitcoinMainnet
      {
        completion(mockBtcBalanceInWei, .success, "")
      } else if coin == .btc,
        chainId == BraveWallet.BitcoinTestnet
      {
        completion(mockBtcTestnetBalanceInWei, .success, "")
      } else if coin == .zec,
        chainId == BraveWallet.ZCashMainnet
      {
        completion(mockZcashBalanceInWei, .success, "")
      } else if coin == .zec,
        chainId == BraveWallet.ZCashTestnet
      {
        completion(mockZcashTestnetBalanceInWei, .success, "")
      } else {
        completion("", .internalError, "")
      }
    }
    rpcService._erc20TokenBalance = { contractAddress, accountAddress, _, completion in
      // usdc balance
      if accountAddress == self.ethAccount1.address {
        completion(usdcAccount1BalanceWei, .success, "")
      } else if accountAddress == self.ethAccount2.address {
        completion(usdcAccount2BalanceWei, .success, "")
      }
    }
    rpcService._solanaBalance = { accountAddress, chainId, completion in
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
    assetRatioService._price = { _, _, completion in
      completion(
        true,
        [
          self.mockETHAssetPrice,
          self.mockUSDCAssetPrice,
          self.mockSOLAssetPrice,
          self.mockFILAssetPrice,
          self.mockFILTestnetAssetPrice,
          self.mockBTCAssetPrice,
          self.mockBTCTestnetAssetPrice,
          self.mockZECAssetPrice,
          self.mockZECTestnetAssetPrice,
        ]
      )
    }

    let userAssetManager = TestableWalletUserAssetManager()
    userAssetManager._getAllUserAssetsInNetworkAssets = { networks, _ in
      [
        NetworkAssets(
          network: .mockMainnet,
          tokens: self.ethMainnetTokens,
          sortOrder: 0
        ),
        NetworkAssets(
          network: .mockSolana,
          tokens: self.solMainnetTokens,
          sortOrder: 1
        ),
        NetworkAssets(
          network: .mockFilecoinMainnet,
          tokens: self.filMainnetTokens,
          sortOrder: 2
        ),
        NetworkAssets(
          network: .mockFilecoinTestnet,
          tokens: self.filTestnetTokens,
          sortOrder: 3
        ),
        NetworkAssets(
          network: .mockBitcoinMainnet,
          tokens: self.btcMainnetTokens,
          sortOrder: 4
        ),
        NetworkAssets(
          network: .mockBitcoinTestnet,
          tokens: self.btcTestnetTokens,
          sortOrder: 5
        ),
        NetworkAssets(
          network: .mockZcashMainnet,
          tokens: self.zcashMainnetTokens,
          sortOrder: 6
        ),
        NetworkAssets(
          network: .mockZcashTestnet,
          tokens: self.zcashTestnetTokens,
          sortOrder: 7
        ),
      ].filter { networkAsset in
        networks.contains(where: { $0.chainId == networkAsset.network.chainId })
      }
    }

    let btcMainnetBalance: UInt64 = 1000
    let btcTestnetBalance: UInt64 = 10000
    let bitcoinWalletService = BraveWallet.TestBitcoinWalletService()
    bitcoinWalletService._balance = { accountId, completion in
      if accountId.uniqueKey == self.btcAccount1.accountId.uniqueKey {
        completion(
          .init(
            totalBalance: btcMainnetBalance,
            availableBalance: btcMainnetBalance,
            pendingBalance: 0,
            balances: [:]
          ),
          nil
        )
      } else {
        completion(
          .init(
            totalBalance: btcTestnetBalance,
            availableBalance: btcTestnetBalance,
            pendingBalance: 0,
            balances: [:]
          ),
          nil
        )
      }
    }
    let zecMainnetBalance: UInt64 = 100_000_0
    let zecTestnetBalance: UInt64 = 100_000
    let zcashWalletService = BraveWallet.TestZCashWalletService()
    zcashWalletService._balance = { accountId, completion in
      if accountId.keyringId == BraveWallet.KeyringId.zCashMainnet {
        completion(
          .init(
            totalBalance: zecMainnetBalance,
            transparentBalance: zecMainnetBalance,
            shieldedBalance: 0,
            shieldedPendingBalance: 0,
            balances: [:]
          ),
          nil
        )
      } else {
        completion(
          .init(
            totalBalance: zecTestnetBalance,
            transparentBalance: zecTestnetBalance,
            shieldedBalance: 0,
            shieldedPendingBalance: 0,
            balances: [:]
          ),
          nil
        )
      }
    }

    let store = AccountsStore(
      keyringService: keyringService,
      rpcService: rpcService,
      walletService: walletService,
      assetRatioService: assetRatioService,
      bitcoinWalletService: bitcoinWalletService,
      zcashWalletService: zcashWalletService,
      userAssetManager: userAssetManager
    )

    let updateExpectation = expectation(description: "update")
    store.$primaryAccounts
      .dropFirst()  // initial
      .collect(2)  // with accounts & tokens, with balances & prices loaded
      .sink { accountDetails in
        defer { updateExpectation.fulfill() }
        guard let accountDetails = accountDetails.last else {
          XCTFail("Expected account details models")
          return
        }
        var accountNumber = 9
        if !bitcoinAndZcashTestnetEnabled {
          accountNumber -= 2
        }
        XCTAssertEqual(accountDetails.count, accountNumber)

        XCTAssertEqual(accountDetails[safe: 0]?.account, self.ethAccount1)
        XCTAssertEqual(accountDetails[safe: 0]?.tokensWithBalance, self.ethMainnetTokens)
        XCTAssertEqual(accountDetails[safe: 0]?.totalBalanceFiat, "$2,741.78")

        XCTAssertEqual(accountDetails[safe: 1]?.account, self.ethAccount2)
        XCTAssertEqual(accountDetails[safe: 1]?.tokensWithBalance.count, 1)  // usdc only
        XCTAssertEqual(
          accountDetails[safe: 1]?.tokensWithBalance[safe: 0],
          self.ethMainnetTokens[safe: 1]
        )  // usdc
        XCTAssertEqual(accountDetails[safe: 1]?.totalBalanceFiat, "$0.01")

        XCTAssertEqual(accountDetails[safe: 2]?.account, self.solAccount1)
        XCTAssertEqual(accountDetails[safe: 2]?.tokensWithBalance, self.solMainnetTokens)
        XCTAssertEqual(accountDetails[safe: 2]?.totalBalanceFiat, "$775.30")

        XCTAssertEqual(accountDetails[safe: 3]?.account, self.filAccount1)
        XCTAssertEqual(accountDetails[safe: 3]?.tokensWithBalance, self.filMainnetTokens)
        XCTAssertEqual(accountDetails[safe: 3]?.totalBalanceFiat, "$4.00")

        XCTAssertEqual(accountDetails[safe: 4]?.account, self.filTestnetAccount)
        XCTAssertEqual(accountDetails[safe: 4]?.tokensWithBalance, self.filTestnetTokens)
        XCTAssertEqual(accountDetails[safe: 4]?.totalBalanceFiat, "$4.00")

        XCTAssertEqual(accountDetails[safe: 5]?.account, self.btcAccount1)
        XCTAssertEqual(accountDetails[safe: 5]?.tokensWithBalance, self.btcMainnetTokens)
        XCTAssertEqual(accountDetails[safe: 5]?.totalBalanceFiat, "$0.657")

        if bitcoinAndZcashTestnetEnabled {
          XCTAssertEqual(accountDetails[safe: 6]?.account, self.btcTestnetAccount)
          XCTAssertEqual(accountDetails[safe: 6]?.tokensWithBalance, self.btcTestnetTokens)
          XCTAssertEqual(accountDetails[safe: 6]?.totalBalanceFiat, "$6.573")

          XCTAssertEqual(accountDetails[safe: 7]?.account, self.zcashAccount1)
          XCTAssertEqual(accountDetails[safe: 7]?.tokensWithBalance, self.zcashMainnetTokens)
          XCTAssertEqual(accountDetails[safe: 7]?.totalBalanceFiat, "$0.365")

          XCTAssertEqual(accountDetails[safe: 8]?.account, self.zcashTestnetAccount)
          XCTAssertEqual(accountDetails[safe: 8]?.tokensWithBalance, self.zcashTestnetTokens)
          XCTAssertEqual(accountDetails[safe: 8]?.totalBalanceFiat, "$0.0365")
        } else {
          XCTAssertEqual(accountDetails[safe: 6]?.account, self.zcashAccount1)
          XCTAssertEqual(accountDetails[safe: 6]?.tokensWithBalance, self.zcashMainnetTokens)
          XCTAssertEqual(accountDetails[safe: 6]?.totalBalanceFiat, "$0.365")
        }
      }.store(in: &cancellables)

    store.update()

    await fulfillment(of: [updateExpectation], timeout: 1)
  }

  func testUpdate() async {
    await updateHelper(bitcoinAndZcashTestnetEnabled: false)
  }

  func testUpdateBitcoinTestnet() async {
    await updateHelper(bitcoinAndZcashTestnetEnabled: true)
  }
}
