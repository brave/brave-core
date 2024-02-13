// Copyright 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Combine
import XCTest
import BraveCore
import Preferences
@testable import BraveWallet

@MainActor class AccountsStoreTests: XCTestCase {
  
  private var cancellables: Set<AnyCancellable> = .init()
  
  let ethAccount1: BraveWallet.AccountInfo = .mockEthAccount
  let ethAccount2 = (BraveWallet.AccountInfo.mockEthAccount.copy() as! BraveWallet.AccountInfo).then {
    $0.address = "mock_eth_id_2"
    $0.name = "Ethereum Account 2"
  }
  let solAccount1: BraveWallet.AccountInfo = .mockSolAccount
  let filAccount1: BraveWallet.AccountInfo = .mockFilAccount
  let filTestnetAccount: BraveWallet.AccountInfo = .mockFilTestnetAccount
  
  let mockETHBalanceAccount1: Double = 0.896
  let mockETHPrice: String = "3059.99"
  lazy var mockETHAssetPrice: BraveWallet.AssetPrice = .init(
    fromAsset: "eth", toAsset: "usd",
    price: mockETHPrice, assetTimeframeChange: "-57.23"
  )
  let mockUSDCBalanceAccount1: Double = 0.03
  let mockUSDCBalanceAccount2: Double = 0.01
  let mockUSDCPrice: String = "1"
  lazy var mockUSDCAssetPrice: BraveWallet.AssetPrice = .init(
    fromAsset: BraveWallet.BlockchainToken.mockUSDCToken.assetRatioId,
    toAsset: "usd", price: mockUSDCPrice, assetTimeframeChange: "-57.23"
  )
  let ethMainnetTokens: [BraveWallet.BlockchainToken] = [
    BraveWallet.NetworkInfo.mockMainnet.nativeToken,
    .mockUSDCToken.copy(asVisibleAsset: true)
  ]
  
  let mockSOLBalance: Double = 3.8765
  let mockSOLPrice: String = "200"
  lazy var mockSOLAssetPrice: BraveWallet.AssetPrice = .init(
    fromAsset: "sol", toAsset: "usd",
    price: mockSOLPrice, assetTimeframeChange: "-57.23"
  )
  let solMainnetTokens: [BraveWallet.BlockchainToken] = [
    BraveWallet.NetworkInfo.mockSolana.nativeToken
  ]
  
  let mockFILBalanceAccount1: Double = 1
  let mockFILTestnetBalanceAccount1: Double = 10
  let mockFILPrice: String = "4.00"
  lazy var mockFILAssetPrice: BraveWallet.AssetPrice = .init(
    fromAsset: "fil", toAsset: "usd",
    price: mockFILPrice, assetTimeframeChange: "-57.23"
  )
  let filMainnetTokens: [BraveWallet.BlockchainToken] = [
    BraveWallet.NetworkInfo.mockFilecoinMainnet.nativeToken
  ]
  let filTestnetTokens: [BraveWallet.BlockchainToken] = [
    BraveWallet.NetworkInfo.mockFilecoinTestnet.nativeToken
  ]
  
  let formatter = WeiFormatter(decimalFormatStyle: .decimals(precision: 18))
  
  func testUpdate() async {
    Preferences.Wallet.showTestNetworks.value = true
    let ethBalanceWei = formatter.weiString(
      from: mockETHBalanceAccount1,
      radix: .hex,
      decimals: Int(BraveWallet.NetworkInfo.mockMainnet.nativeToken.decimals)
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
    let mockSOLLamportBalance: UInt64 = 3876535000 // ~3.8765 SOL
    let mockFilBalanceInWei = formatter.weiString(
      from: mockFILBalanceAccount1,
      radix: .decimal,
      decimals: Int(BraveWallet.NetworkInfo.mockFilecoinMainnet.nativeToken.decimals)
    ) ?? ""
    let mockFilTestnetBalanceInWei = formatter.weiString(
      from: mockFILBalanceAccount1,
      radix: .decimal,
      decimals: Int(BraveWallet.NetworkInfo.mockFilecoinTestnet.nativeToken.decimals)
    ) ?? ""
    
    let keyringService = BraveWallet.TestKeyringService()
    keyringService._addObserver = { _ in }
    keyringService._allAccounts = {
      $0(.init(
        accounts: [
          self.ethAccount1, self.ethAccount2,
          self.solAccount1, self.filAccount1,
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
      completion([
        .mockMainnet,
        .mockSolana,
        .mockFilecoinMainnet,
        .mockFilecoinTestnet
      ].filter { $0.coin == coin })
    }
    rpcService._balance = { accountAddress, coin, chainId, completion in
      if coin == .eth,
         chainId == BraveWallet.MainnetChainId,
         accountAddress == self.ethAccount1.address {
        completion(ethBalanceWei, .success, "")
      } else if coin == .fil,
                chainId == BraveWallet.FilecoinMainnet,
                accountAddress == self.filAccount1.address { // .fil
        completion(mockFilBalanceInWei, .success, "")
      } else if coin == .fil,
                chainId == BraveWallet.FilecoinTestnet,
                accountAddress == self.filTestnetAccount.address {
        completion(mockFilTestnetBalanceInWei, .success, "")
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
    rpcService._erc721TokenBalance = { _, _, _, _, completion in
      completion("", .internalError, "")
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
    
    let assetRatioService = BraveWallet.TestAssetRatioService()
    assetRatioService._price = { priceIds, _, _, completion in
      completion(true, [self.mockETHAssetPrice,
                        self.mockUSDCAssetPrice,
                        self.mockSOLAssetPrice,
                        self.mockFILAssetPrice])
    }
    
    let userAssetManager = TestableWalletUserAssetManager()
    userAssetManager._getAllUserAssetsInNetworkAssets = { networks, _ in
      [
        NetworkAssets(
          network: .mockMainnet,
          tokens: self.ethMainnetTokens,
          sortOrder: 0),
        NetworkAssets(
          network: .mockSolana,
          tokens: self.solMainnetTokens,
          sortOrder: 1),
        NetworkAssets(
          network: .mockFilecoinMainnet,
          tokens: self.filMainnetTokens,
          sortOrder: 2),
        NetworkAssets(
          network: .mockFilecoinTestnet,
          tokens: self.filTestnetTokens,
          sortOrder: 3)
      ].filter { networkAsset in
        networks.contains(where: { $0.chainId == networkAsset.network.chainId })
      }
    }
    
    let store = AccountsStore(
      keyringService: keyringService,
      rpcService: rpcService,
      walletService: walletService,
      assetRatioService: assetRatioService,
      userAssetManager: userAssetManager
    )
    
    let updateExpectation = expectation(description: "update")
    store.$primaryAccounts
      .dropFirst() // initial
      .collect(2) // with accounts & tokens, with balances & prices loaded
      .sink { accountDetails in
        defer { updateExpectation.fulfill() }
        guard let accountDetails = accountDetails.last else {
          XCTFail("Expected account details models")
          return
        }
        XCTAssertEqual(accountDetails.count, 5)
        
        XCTAssertEqual(accountDetails[safe: 0]?.account, self.ethAccount1)
        XCTAssertEqual(accountDetails[safe: 0]?.tokensWithBalance, self.ethMainnetTokens)
        XCTAssertEqual(accountDetails[safe: 0]?.totalBalanceFiat, "$2,741.78")
        
        XCTAssertEqual(accountDetails[safe: 1]?.account, self.ethAccount2)
        XCTAssertEqual(accountDetails[safe: 1]?.tokensWithBalance.count, 1) // usdc only
        XCTAssertEqual(accountDetails[safe: 1]?.tokensWithBalance[safe: 0],
                       self.ethMainnetTokens[safe: 1]) // usdc
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
      }.store(in: &cancellables)
    
    store.update()
    
    await fulfillment(of: [updateExpectation], timeout: 1)
  }
  
  override class func tearDown() {
    super.tearDown()
    Preferences.Wallet.showTestNetworks.reset()
  }
}
