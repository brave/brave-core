// Copyright 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import XCTest
import BraveCore
import Combine
import BigNumber
@testable import BraveWallet

class SwapStoreTests: XCTestCase {
  private var cancellables: Set<AnyCancellable> = []
  
  func testDefaultSellBuyTokensOnMainnetWithoutPrefilledToken() {
    let store = SwapTokenStore(
      keyringService: MockKeyringService(),
      blockchainRegistry: MockBlockchainRegistry(),
      rpcService: MockJsonRpcService(),
      swapService: MockSwapService(),
      txService: MockTxService(),
      walletService: MockBraveWalletService(),
      ethTxManagerProxy: MockEthTxManagerProxy(),
      prefilledToken: nil
    )
    let ex = expectation(description: "default-sell-buy-token-on-main")
    XCTAssertNil(store.selectedFromToken)
    XCTAssertNil(store.selectedToToken)
    let testAccountInfo: BraveWallet.AccountInfo = .init()
    store.prepare(with: testAccountInfo) {
      defer { ex.fulfill() }
      XCTAssertEqual(store.selectedFromToken?.symbol.lowercased(), "eth")
      XCTAssertEqual(store.selectedToToken?.symbol.lowercased(), "bat")
    }
    waitForExpectations(timeout: 3) { error in
      XCTAssertNil(error)
    }
  }

  func testDefaultSellBuyTokensOnMainnetWithPrefilledToken() {
    let batToken: BraveWallet.BlockchainToken = .init(contractAddress: "", name: "Brave BAT", logo: "", isErc20: true, isErc721: false, isNft: false, symbol: "BAT", decimals: 18, visible: false, tokenId: "", coingeckoId: "", chainId: BraveWallet.MainnetChainId, coin: .eth)
    let store = SwapTokenStore(
      keyringService: MockKeyringService(),
      blockchainRegistry: MockBlockchainRegistry(),
      rpcService: MockJsonRpcService(),
      swapService: MockSwapService(),
      txService: MockTxService(),
      walletService: MockBraveWalletService(),
      ethTxManagerProxy: MockEthTxManagerProxy(),
      prefilledToken: batToken
    )
    let fromTokenExpectation = expectation(description: "update-fromTokenExpectation")
    store.$selectedFromToken
      .dropFirst()
      .first()
      .sink { selectedFromToken in
        defer { fromTokenExpectation.fulfill() }
        XCTAssertEqual(selectedFromToken, batToken)
      }
      .store(in: &cancellables)
    let ex = expectation(description: "default-sell-buy-token-on-main")
    XCTAssertNil(store.selectedFromToken) // `prefilledToken` not set until validated in `prepare()`
    XCTAssertNil(store.selectedToToken)
    let testAccountInfo: BraveWallet.AccountInfo = .init()
    store.prepare(with: testAccountInfo) {
      defer { ex.fulfill() }
      XCTAssertEqual(store.selectedFromToken?.symbol.lowercased(), batToken.symbol.lowercased())
      XCTAssertNotNil(store.selectedToToken)
      XCTAssertNotEqual(store.selectedToToken!.symbol.lowercased(), batToken.symbol.lowercased())
    }
    waitForExpectations(timeout: 3) { error in
      XCTAssertNil(error)
    }
  }
  
  /// Test that given a `prefilledToken` that is not on the current network, the `BuyTokenStore`
  /// will switch networks to the `chainId` of the token.
  func testPrefilledTokenSwitchNetwork() {
    let prefilledToken: BraveWallet.BlockchainToken = .mockSolToken
    
    var selectedCoin: BraveWallet.CoinType = .eth
    var selectedNetwork: BraveWallet.NetworkInfo = .mockMainnet
    let walletService = BraveWallet.TestBraveWalletService()
    walletService._addObserver = { _ in }
    walletService._selectedCoin = { $0(selectedCoin) }
    walletService._userAssets = { _, coin, completion in
      completion(coin == .eth ? [.previewToken] : [.mockSolToken])
    }
    let rpcService = BraveWallet.TestJsonRpcService()
    rpcService._addObserver = { _ in }
    rpcService._network = { coin, completion in
      completion(selectedNetwork)
    }
    rpcService._solanaBalance = { _, _, completion in
      completion(0, .success, "")
    }
    rpcService._splTokenAccountBalance = { _, _, _, completion in
      completion("", 0, "", .success, "")
    }
    rpcService._allNetworks = { coin, completion in
      completion(coin == .eth ? [.mockMainnet] : [.mockSolana])
    }
    // simulate network switch when `setNetwork` is called
    rpcService._setNetwork = { chainId, coin, completion in
      XCTAssertEqual(chainId, BraveWallet.SolanaMainnet) // verify network switched to SolanaMainnet
      selectedCoin = coin
      selectedNetwork = coin == .eth ? .mockMainnet : .mockSolana
      completion(true)
    }
    
    let store = SwapTokenStore(
      keyringService: MockKeyringService(),
      blockchainRegistry: MockBlockchainRegistry(),
      rpcService: rpcService,
      swapService: MockSwapService(),
      txService: MockTxService(),
      walletService: walletService,
      ethTxManagerProxy: MockEthTxManagerProxy(),
      prefilledToken: prefilledToken
    )
    
    let fromTokenExpectation = expectation(description: "update-fromTokenExpectation")
    store.$selectedFromToken
      .dropFirst()
      .first()
      .sink { selectedFromToken in
        defer { fromTokenExpectation.fulfill() }
        XCTAssertEqual(selectedFromToken, .mockSolToken)
      }
      .store(in: &cancellables)
    let ex = expectation(description: "default-sell-buy-token-on-main")
    XCTAssertNil(store.selectedFromToken) // `prefilledToken` not set until validated in `prepare()`
    XCTAssertNil(store.selectedToToken)
    let testAccountInfo: BraveWallet.AccountInfo = .init()
    store.prepare(with: testAccountInfo) {
      defer { ex.fulfill() }
      XCTAssertEqual(store.selectedFromToken?.symbol.lowercased(), prefilledToken.symbol.lowercased())
      XCTAssertNotNil(store.selectedToToken)
      XCTAssertNotEqual(store.selectedToToken!.symbol.lowercased(), prefilledToken.symbol.lowercased())
    }
    waitForExpectations(timeout: 3) { error in
      XCTAssertNil(error)
    }
  }

  func testDefaultSellBuyTokensOnEVMWithoutPrefilledToken() {
    let rpcService = MockJsonRpcService()
    let store = SwapTokenStore(
      keyringService: MockKeyringService(),
      blockchainRegistry: MockBlockchainRegistry(),
      rpcService: rpcService,
      swapService: MockSwapService(),
      txService: MockTxService(),
      walletService: MockBraveWalletService(),
      ethTxManagerProxy: MockEthTxManagerProxy(),
      prefilledToken: nil
    )
    let ex = expectation(description: "default-sell-buy-token-on-evm")
    XCTAssertNil(store.selectedFromToken)
    XCTAssertNil(store.selectedToToken)
    
    rpcService.setNetwork(BraveWallet.PolygonMainnetChainId, coin: .eth) { success in
      XCTAssertTrue(success)
      let testAccountInfo: BraveWallet.AccountInfo = .init()
      store.prepare(with: testAccountInfo) {
        defer { ex.fulfill() }
        XCTAssertEqual(store.selectedFromToken?.symbol.lowercased(), "matic")
        XCTAssertEqual(store.selectedToToken?.symbol.lowercased(), "bat")
      }
    }
    waitForExpectations(timeout: 3) { error in
      XCTAssertNil(error)
    }
  }

  func testDefaultSellBuyTokensOnEVMWithPrefilledToken() {
    let daiToken: BraveWallet.BlockchainToken = .init(contractAddress: "", name: "DAI Stablecoin", logo: "", isErc20: true, isErc721: false, isNft: false, symbol: "DAI", decimals: 18, visible: false, tokenId: "", coingeckoId: "", chainId: BraveWallet.PolygonMainnetChainId, coin: .eth)
    let batToken: BraveWallet.BlockchainToken = .init(contractAddress: "0x0d8775f648430679a709e98d2b0cb6250d2887ef", name: "Basic Attention Token", logo: "", isErc20: true, isErc721: false, isNft: false, symbol: "BAT", decimals: 18, visible: true, tokenId: "", coingeckoId: "", chainId: BraveWallet.PolygonMainnetChainId, coin: .eth)
    let rpcService = BraveWallet.TestJsonRpcService()
    rpcService._addObserver = { _ in }
    rpcService._network = { $1(.mockPolygon) }
    rpcService._erc20TokenBalance = { $3("10", .success, "") }
    let store = SwapTokenStore(
      keyringService: MockKeyringService(),
      blockchainRegistry: MockBlockchainRegistry(),
      rpcService: rpcService,
      swapService: MockSwapService(),
      txService: MockTxService(),
      walletService: MockBraveWalletService(),
      ethTxManagerProxy: MockEthTxManagerProxy(),
      prefilledToken: daiToken
    )
    XCTAssertNil(store.selectedFromToken)  // `prefilledToken` not set until validated in `prepare()`
    XCTAssertNil(store.selectedToToken)
    let fromTokenExpectation = expectation(description: "update-sendTokenExpectation")
    store.$selectedFromToken
      .dropFirst()
      .first()
      .sink { selectedFromToken in
        defer { fromTokenExpectation.fulfill() }
        XCTAssertEqual(selectedFromToken, daiToken)
      }
      .store(in: &cancellables)

    let ex = expectation(description: "default-sell-buy-token-on-evm")
    let testAccountInfo: BraveWallet.AccountInfo = .init()
    store.prepare(with: testAccountInfo) {
      defer { ex.fulfill() }
      XCTAssertEqual(store.selectedFromToken?.symbol.lowercased(), daiToken.symbol.lowercased())
      XCTAssertNotNil(store.selectedToToken)
      XCTAssertEqual(store.selectedToToken!.symbol.lowercased(), batToken.symbol.lowercased())
    }
    waitForExpectations(timeout: 3) { error in
      XCTAssertNil(error)
    }
  }

  func testFetchPriceQuote() {
    let store = SwapTokenStore(
      keyringService: MockKeyringService(),
      blockchainRegistry: MockBlockchainRegistry(),
      rpcService: MockJsonRpcService(),
      swapService: MockSwapService(),
      txService: MockTxService(),
      walletService: MockBraveWalletService(),
      ethTxManagerProxy: MockEthTxManagerProxy(),
      prefilledToken: nil
    )
    let ex = expectation(description: "fetch-price-quote")
    store.setUpTest()
    DispatchQueue.main.asyncAfter(deadline: .now() + 1.0) {
      if case .swap = store.state {
        ex.fulfill()
      }
    }
    waitForExpectations(timeout: 3) { error in
      XCTAssertNil(error)
    }
  }

  func testSwapERC20EIP1559Transaction() {
    let store = SwapTokenStore(
      keyringService: MockKeyringService(),
      blockchainRegistry: MockBlockchainRegistry(),
      rpcService: MockJsonRpcService(),
      swapService: MockSwapService(),
      txService: MockTxService(),
      walletService: MockBraveWalletService(),
      ethTxManagerProxy: MockEthTxManagerProxy(),
      prefilledToken: nil
    )
    let ex = expectation(description: "make-erc20-eip1559-swap-transaction")
    store.setUpTest()
    store.state = .lowAllowance("test-spender-address")
    store.prepareSwap { _ in }
    DispatchQueue.main.asyncAfter(deadline: .now() + 1.0) {
      if case .swap = store.state {
        ex.fulfill()
      }
    }
    waitForExpectations(timeout: 3) { error in
      XCTAssertNil(error)
    }
  }

  func testSwapERC20Transaction() {
    let rpcService = MockJsonRpcService()
    let store = SwapTokenStore(
      keyringService: MockKeyringService(),
      blockchainRegistry: MockBlockchainRegistry(),
      rpcService: rpcService,
      swapService: MockSwapService(),
      txService: MockTxService(),
      walletService: MockBraveWalletService(),
      ethTxManagerProxy: MockEthTxManagerProxy(),
      prefilledToken: nil
    )
    let ex = expectation(description: "make-erc20-swap-transaction")
    store.setUpTest()
    store.state = .lowAllowance("test-spender-address")

    rpcService.setNetwork(BraveWallet.GoerliChainId, coin: .eth) { success in
      XCTAssertTrue(success)
      store.prepareSwap { _ in }
      DispatchQueue.main.asyncAfter(deadline: .now() + 1.0) {
        if case .swap = store.state {
          ex.fulfill()
        }
      }
    }
    waitForExpectations(timeout: 3) { error in
      XCTAssertNil(error)
    }
  }

  func testSwapETHSwapEIP1559Transaction() {
    let store = SwapTokenStore(
      keyringService: MockKeyringService(),
      blockchainRegistry: MockBlockchainRegistry(),
      rpcService: MockJsonRpcService(),
      swapService: MockSwapService(),
      txService: MockTxService(),
      walletService: MockBraveWalletService(),
      ethTxManagerProxy: MockEthTxManagerProxy(),
      prefilledToken: nil
    )
    let ex = expectation(description: "make-eth-swap-eip1559-transaction")
    store.setUpTest()
    store.state = .swap
    store.prepareSwap { _ in }
    DispatchQueue.main.asyncAfter(deadline: .now() + 1.0) {
      if case .swap = store.state {
        ex.fulfill()
      }
    }
    waitForExpectations(timeout: 3) { error in
      XCTAssertNil(error)
    }
  }

  func testSwapETHSwapTransaction() {
    let rpcService = MockJsonRpcService()
    let store = SwapTokenStore(
      keyringService: MockKeyringService(),
      blockchainRegistry: MockBlockchainRegistry(),
      rpcService: rpcService,
      swapService: MockSwapService(),
      txService: MockTxService(),
      walletService: MockBraveWalletService(),
      ethTxManagerProxy: MockEthTxManagerProxy(),
      prefilledToken: nil
    )
    let ex = expectation(description: "make-eth-swap-eip1559-transaction")
    store.setUpTest()
    store.state = .swap

    rpcService.setNetwork(BraveWallet.GoerliChainId, coin: .eth) { success in
      XCTAssertTrue(success)
      store.prepareSwap { _ in }
      DispatchQueue.main.asyncAfter(deadline: .now() + 1.0) {
        if case .swap = store.state {
          ex.fulfill()
        }
      }
    }
    waitForExpectations(timeout: 3) { error in
      XCTAssertNil(error)
    }
  }
  
  func testSwapFullBalanceNoRounding() {
    let formatter = WeiFormatter(decimalFormatStyle: .decimals(precision: 18))
    let mockBalance = "47.156499657504857477"
    let mockBalanceWei = formatter.weiString(from: mockBalance, radix: .hex, decimals: 18) ?? ""
    
    let rpcService = BraveWallet.TestJsonRpcService()
    rpcService._chainId = { $1(BraveWallet.NetworkInfo.mockGoerli.chainId) }
    rpcService._network = { $1(BraveWallet.NetworkInfo.mockGoerli)}
    rpcService._balance = { _, _, _, completion in
      completion(mockBalanceWei, .success, "")
    }
    rpcService._addObserver = { _ in }
    
    let store = SwapTokenStore(
      keyringService: MockKeyringService(),
      blockchainRegistry: MockBlockchainRegistry(),
      rpcService: rpcService,
      swapService: MockSwapService(),
      txService: MockTxService(),
      walletService: MockBraveWalletService(),
      ethTxManagerProxy: MockEthTxManagerProxy(),
      prefilledToken: nil
    )
    store.setUpTestForRounding()
    
    let fetchFromTokenBalanceEx = expectation(description: "fetchFromTokenBalance")
    store.$selectedFromTokenBalance
      .sink { balance in
        XCTAssertEqual(balance, BDouble(mockBalance)!)
        store.suggestedAmountTapped(.all)
        fetchFromTokenBalanceEx.fulfill()
      }
      .store(in: &cancellables)
    
    let sendFullBalanceEx = expectation(description: "sendFullBalance")
    store.$sellAmount
      .sink { amount in
        XCTAssertEqual("\(amount)", "\(mockBalance)")
        sendFullBalanceEx.fulfill()
      }
      .store(in: &cancellables)
    
    waitForExpectations(timeout: 3) { error in
      XCTAssertNil(error)
    }
  }
}
