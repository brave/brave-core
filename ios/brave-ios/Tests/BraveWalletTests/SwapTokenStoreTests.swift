// Copyright 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BigNumber
import BraveCore
import Combine
import XCTest

@testable import BraveWallet

class SwapStoreTests: XCTestCase {
  private var cancellables: Set<AnyCancellable> = []

  func testDefaultSellBuyTokensOnMainnetWithoutPrefilledToken() {
    let solTxManagerProxy = BraveWallet.TestSolanaTxManagerProxy()
    let store = SwapTokenStore(
      keyringService: MockKeyringService(),
      blockchainRegistry: MockBlockchainRegistry(),
      rpcService: MockJsonRpcService(),
      swapService: MockSwapService(),
      txService: MockTxService(),
      walletService: MockBraveWalletService(),
      ethTxManagerProxy: MockEthTxManagerProxy(),
      solTxManagerProxy: solTxManagerProxy,
      userAssetManager: TestableWalletUserAssetManager(),
      prefilledToken: nil
    )
    let ex = expectation(description: "default-sell-buy-token-on-main")
    XCTAssertNil(store.selectedFromToken)
    XCTAssertNil(store.selectedToToken)
    let testAccountInfo: BraveWallet.AccountInfo = .previewAccount
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
    let batToken: BraveWallet.BlockchainToken = .init(
      contractAddress: "",
      name: "Brave BAT",
      logo: "",
      isCompressed: false,
      isErc20: true,
      isErc721: false,
      isErc1155: false,
      splTokenProgram: .unsupported,
      isNft: false,
      isSpam: false,
      symbol: "BAT",
      decimals: 18,
      visible: false,
      tokenId: "",
      coingeckoId: "",
      chainId: BraveWallet.MainnetChainId,
      coin: .eth,
      isShielded: false
    )
    let solTxManagerProxy = BraveWallet.TestSolanaTxManagerProxy()
    let store = SwapTokenStore(
      keyringService: MockKeyringService(),
      blockchainRegistry: MockBlockchainRegistry(),
      rpcService: MockJsonRpcService(),
      swapService: MockSwapService(),
      txService: MockTxService(),
      walletService: MockBraveWalletService(),
      ethTxManagerProxy: MockEthTxManagerProxy(),
      solTxManagerProxy: solTxManagerProxy,
      userAssetManager: TestableWalletUserAssetManager(),
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
    // `prefilledToken` not set until validated in `prepare()`
    XCTAssertNil(store.selectedFromToken)
    XCTAssertNil(store.selectedToToken)
    let testAccountInfo: BraveWallet.AccountInfo = .previewAccount
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

    let (
      keyringService, blockchainRegistry, rpcService, swapService, txService, walletService,
      ethTxManagerProxy, solTxManagerProxy, mockAssetManager
    ) = setupServices()
    var selectedNetwork: BraveWallet.NetworkInfo = .mockMainnet

    rpcService._network = { coin, _, completion in
      completion(selectedNetwork)
    }
    // simulate network switch when `setNetwork` is called
    rpcService._setNetwork = { chainId, coin, origin, completion in
      // verify network switched to SolanaMainnet
      XCTAssertEqual(chainId, BraveWallet.SolanaMainnet)
      selectedNetwork = coin == .eth ? .mockMainnet : .mockSolana
      completion(true)
    }

    let store = SwapTokenStore(
      keyringService: keyringService,
      blockchainRegistry: blockchainRegistry,
      rpcService: rpcService,
      swapService: swapService,
      txService: txService,
      walletService: walletService,
      ethTxManagerProxy: ethTxManagerProxy,
      solTxManagerProxy: solTxManagerProxy,
      userAssetManager: mockAssetManager,
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
    // `prefilledToken` not set until validated in `prepare()`
    XCTAssertNil(store.selectedFromToken)
    XCTAssertNil(store.selectedToToken)
    let testAccountInfo: BraveWallet.AccountInfo = .previewAccount
    store.prepare(with: testAccountInfo) {
      defer { ex.fulfill() }
      XCTAssertEqual(
        store.selectedFromToken?.symbol.lowercased(),
        prefilledToken.symbol.lowercased()
      )
      XCTAssertNotNil(store.selectedToToken)
      XCTAssertNotEqual(
        store.selectedToToken?.symbol.lowercased(),
        prefilledToken.symbol.lowercased()
      )
    }
    waitForExpectations(timeout: 3) { error in
      XCTAssertNil(error)
    }
  }

  func testDefaultSellBuyTokensOnEVMWithoutPrefilledToken() {
    let rpcService = MockJsonRpcService()
    let solTxManagerProxy = BraveWallet.TestSolanaTxManagerProxy()
    let store = SwapTokenStore(
      keyringService: MockKeyringService(),
      blockchainRegistry: MockBlockchainRegistry(),
      rpcService: rpcService,
      swapService: MockSwapService(),
      txService: MockTxService(),
      walletService: MockBraveWalletService(),
      ethTxManagerProxy: MockEthTxManagerProxy(),
      solTxManagerProxy: solTxManagerProxy,
      userAssetManager: TestableWalletUserAssetManager(),
      prefilledToken: nil
    )
    let ex = expectation(description: "default-sell-buy-token-on-evm")
    XCTAssertNil(store.selectedFromToken)
    XCTAssertNil(store.selectedToToken)

    rpcService.setNetwork(chainId: BraveWallet.PolygonMainnetChainId, coin: .eth, origin: nil) {
      success in
      XCTAssertTrue(success)
      let testAccountInfo: BraveWallet.AccountInfo = .previewAccount
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
    let daiToken: BraveWallet.BlockchainToken = .init(
      contractAddress: "",
      name: "DAI Stablecoin",
      logo: "",
      isCompressed: false,
      isErc20: true,
      isErc721: false,
      isErc1155: false,
      splTokenProgram: .unsupported,
      isNft: false,
      isSpam: false,
      symbol: "DAI",
      decimals: 18,
      visible: false,
      tokenId: "",
      coingeckoId: "",
      chainId: BraveWallet.PolygonMainnetChainId,
      coin: .eth,
      isShielded: false
    )
    let batToken: BraveWallet.BlockchainToken = .init(
      contractAddress: "0x0d8775f648430679a709e98d2b0cb6250d2887ef",
      name: "Basic Attention Token",
      logo: "",
      isCompressed: false,
      isErc20: true,
      isErc721: false,
      isErc1155: false,
      splTokenProgram: .unsupported,
      isNft: false,
      isSpam: false,
      symbol: "BAT",
      decimals: 18,
      visible: true,
      tokenId: "",
      coingeckoId: "",
      chainId: BraveWallet.PolygonMainnetChainId,
      coin: .eth,
      isShielded: false
    )
    let rpcService = MockJsonRpcService()
    rpcService._network = { $2(.mockPolygon) }
    rpcService._erc20TokenBalance = { $3("10", .success, "") }
    let solTxManagerProxy = BraveWallet.TestSolanaTxManagerProxy()
    let store = SwapTokenStore(
      keyringService: MockKeyringService(),
      blockchainRegistry: MockBlockchainRegistry(),
      rpcService: rpcService,
      swapService: MockSwapService(),
      txService: MockTxService(),
      walletService: MockBraveWalletService(),
      ethTxManagerProxy: MockEthTxManagerProxy(),
      solTxManagerProxy: solTxManagerProxy,
      userAssetManager: TestableWalletUserAssetManager(),
      prefilledToken: daiToken
    )
    // `prefilledToken` not set until validated in `prepare()`
    XCTAssertNil(store.selectedFromToken)
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
    let testAccountInfo: BraveWallet.AccountInfo = .previewAccount
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

  private func setupServices(
    network: BraveWallet.NetworkInfo = .mockMainnet,
    coin: BraveWallet.CoinType = .eth
  ) -> (
    BraveWallet.TestKeyringService, BraveWallet.TestBlockchainRegistry,
    BraveWallet.TestJsonRpcService, BraveWallet.TestSwapService, BraveWallet.TestTxService,
    BraveWallet.TestBraveWalletService, BraveWallet.TestEthTxManagerProxy,
    BraveWallet.TestSolanaTxManagerProxy, WalletUserAssetManagerType
  ) {
    let keyringService = BraveWallet.TestKeyringService()
    keyringService._addObserver = { _ in }
    let blockchainRegistry = BraveWallet.TestBlockchainRegistry()
    blockchainRegistry._allTokens = { $2([.previewToken, .previewDaiToken]) }
    let rpcService = MockJsonRpcService()
    rpcService.hiddenNetworks.removeAll()
    rpcService._network = { $2(network) }
    rpcService._balance = { _, _, _, completion in
      // return fake sufficient ETH balance `0x13e25e19dc20ba7` is about 0.0896 ETH
      completion("0x13e25e19dc20ba7", .success, "")
    }
    rpcService._erc20TokenBalance = { _, _, _, completion in
      // return fake sufficient ETH balance `0x13e25e19dc20ba7` is about 0.0896 ETH
      completion("0x13e25e19dc20ba7", .success, "")
    }
    let swapService = BraveWallet.TestSwapService()
    swapService._quote = { _, completion in
      if coin == .eth {
        completion(.init(zeroExQuote: .init()), nil, nil, "")
      } else if coin == .sol {
        completion(.init(jupiterQuote: .init()), nil, nil, "")
      } else {
        XCTFail("Coin type is not supported for swap")
      }
    }
    swapService._transaction = { params, completion in
      if coin == .eth {
        if params.zeroExTransactionParams != nil {
          completion(.init(zeroExTransaction: .init()), nil, "")
        } else if params.lifiTransactionParams != nil {
          completion(.init(lifiTransaction: .init(evmTransaction: .init())), nil, "")
        } else {
          XCTFail("Unexpected swap transaction params")
        }
      } else if coin == .sol {
        completion(.init(jupiterTransaction: .init()), nil, "")
      } else {
        XCTFail("Coin type is not supported for swap")
      }
    }
    let txService = BraveWallet.TestTxService()
    txService._addUnapprovedTransaction = { $3(true, "tx-meta-id", "") }
    let walletService = BraveWallet.TestBraveWalletService()
    let mockAssetManager = TestableWalletUserAssetManager()
    mockAssetManager._getAllUserAssetsInNetworkAssets = { _, _ in
      network.coin == .eth
        ? [
          NetworkAssets(
            network: .mockMainnet,
            tokens: [.previewToken, .previewDaiToken],
            sortOrder: 0
          )
        ]
        : [
          NetworkAssets(network: .mockSolana, tokens: [.mockSolToken, .mockSpdToken], sortOrder: 0)
        ]
    }
    let ethTxManagerProxy = BraveWallet.TestEthTxManagerProxy()
    ethTxManagerProxy._makeErc20ApproveData = { $2(true, []) }
    ethTxManagerProxy._gasEstimation1559 = { $1(.init()) }
    let solTxManagerProxy = BraveWallet.TestSolanaTxManagerProxy()
    return (
      keyringService, blockchainRegistry, rpcService, swapService, txService, walletService,
      ethTxManagerProxy, solTxManagerProxy, mockAssetManager
    )
  }

  /// Test change to `sellAmount` (from value) will fetch ZeroEx price quote and assign to `buyAmount`
  func testFetchZeroExPriceQuoteSell() {
    let (
      keyringService, blockchainRegistry, rpcService, swapService, txService, walletService,
      ethTxManagerProxy, solTxManagerProxy, mockAssetManager
    ) = setupServices()
    let zeroExQuote: BraveWallet.ZeroExQuote = .init(
      price: "",
      guaranteedPrice: "",
      to: "",
      data: "",
      value: "",
      gas: "",
      estimatedGas: "",
      gasPrice: "",
      protocolFee: "",
      minimumProtocolFee: "",
      buyTokenAddress: "",
      sellTokenAddress: "",
      buyAmount: "2000000000000000000",
      sellAmount: "",
      allowanceTarget: "",
      sellTokenToEthRate: "",
      buyTokenToEthRate: "",
      estimatedPriceImpact: "",
      sources: [],
      fees: .init(
        zeroExFee: .init(
          feeType: "",
          feeToken: "",
          feeAmount: "",
          billingType: ""
        )
      )
    )
    swapService._quote = { _, completion in
      completion(.init(zeroExQuote: zeroExQuote), .mockEthFees, nil, "")
    }
    let store = SwapTokenStore(
      keyringService: keyringService,
      blockchainRegistry: blockchainRegistry,
      rpcService: rpcService,
      swapService: swapService,
      txService: txService,
      walletService: walletService,
      ethTxManagerProxy: ethTxManagerProxy,
      solTxManagerProxy: solTxManagerProxy,
      userAssetManager: mockAssetManager,
      prefilledToken: nil
    )

    let buyAmountExpectation = expectation(description: "buyAmountExpectation")
    store.$buyAmount
      .dropFirst()
      .collect(3)
      .sink { buyAmounts in
        defer { buyAmountExpectation.fulfill() }
        guard let buyAmount = buyAmounts.last else {
          XCTFail("Expected multiple buyAmount assignments.")
          return
        }
        XCTAssertFalse(buyAmount.isEmpty)
        XCTAssertEqual(buyAmount, "2.0000")
      }
      .store(in: &cancellables)

    XCTAssertTrue(store.buyAmount.isEmpty)
    // non-empty assignment to `sellAmount` calls fetchPriceQuote
    store.setUpTest(sellAmount: "0.01")
    waitForExpectations(timeout: 2) { error in
      XCTAssertNil(error)
      // Verify fees
      XCTAssertEqual(store.braveFeeForDisplay, "0.875%")
    }
  }

  /// Test change to `buyAmount` (to value) will fetch ZeroEx price quote and assign to `buyAmount`
  func testFetchZeroExPriceQuoteBuy() {
    let (
      keyringService, blockchainRegistry, rpcService, swapService, txService, walletService,
      ethTxManagerProxy, solTxManagerProxy, mockAssetManager
    ) = setupServices()
    let zeroExQuote: BraveWallet.ZeroExQuote = .init()
    zeroExQuote.sellAmount = "3000000000000000000"
    swapService._quote = { _, completion in
      completion(.init(zeroExQuote: zeroExQuote), .mockEthFees, nil, "")
    }
    let store = SwapTokenStore(
      keyringService: keyringService,
      blockchainRegistry: blockchainRegistry,
      rpcService: rpcService,
      swapService: swapService,
      txService: txService,
      walletService: walletService,
      ethTxManagerProxy: ethTxManagerProxy,
      solTxManagerProxy: solTxManagerProxy,
      userAssetManager: mockAssetManager,
      prefilledToken: nil
    )

    let sellAmountExpectation = expectation(description: "sellAmountExpectation")
    store.$sellAmount
      .dropFirst()
      .collect(3)
      .sink { sellAmounts in
        defer { sellAmountExpectation.fulfill() }
        guard let sellAmount = sellAmounts.last else {
          XCTFail("Expected multiple sellAmount assignments.")
          return
        }
        XCTAssertFalse(sellAmount.isEmpty)
        XCTAssertEqual(sellAmount, "3.0000")
      }
      .store(in: &cancellables)

    let currentSwapQuoteInfoExpectation = expectation(
      description: "currentSwapQuoteInfoExpectation"
    )
    store.$currentSwapQuoteInfo
      .dropFirst()  // initial state
      .collect(2)  // fetchPriceQuote (nil), fetchEthPriceQuote (populated)
      .sink { currentSwapQuoteInfos in
        guard let currentSwapQuoteInfo = currentSwapQuoteInfos.last else {
          XCTFail("Expected multiple assignments.")
          return
        }
        XCTAssertNotNil(currentSwapQuoteInfo?.swapQuote)
        XCTAssertNotNil(currentSwapQuoteInfo?.swapFees)
        currentSwapQuoteInfoExpectation.fulfill()
      }
      .store(in: &cancellables)

    XCTAssertTrue(store.sellAmount.isEmpty)
    // calls fetchPriceQuote
    store.setUpTest(sellAmount: nil, buyAmount: "0.01")
    waitForExpectations(timeout: 2) { error in
      XCTAssertNil(error)
      // Verify fees
      XCTAssertEqual(store.braveFeeForDisplay, "0.875%")
    }
  }

  /// Test change to `sellAmount` (from value) will fetch Jupiter price quote and assign to `buyAmount` on Solana Mainnet
  func testFetchJupiterPriceQuoteSell() {
    let (
      keyringService, blockchainRegistry, rpcService, swapService, txService, walletService,
      ethTxManagerProxy, solTxManagerProxy, mockAssetManager
    ) = setupServices(
      network: .mockSolana,
      coin: .sol
    )
    swapService._quote = { swapQuoteParams, completion in
      // verify 0.005 is converted to 0.5
      XCTAssertEqual(swapQuoteParams.slippagePercentage, "0.5")
      let jupiterQuote: BraveWallet.JupiterQuote = .init(
        inputMint: BraveWallet.BlockchainToken.mockSolToken.contractAddress,
        inAmount: "10000000",  // 0.01 SOL (9 decimals)
        outputMint: BraveWallet.BlockchainToken.mockSpdToken.contractAddress,
        outAmount: "2500000",  // 2.5 SPD (6 decimals)
        otherAmountThreshold: "2500000",  // 2.5 SPD (6 decimals)
        swapMode: "",
        slippageBps: "50",  // 0.5%
        platformFee: nil,
        priceImpactPct: "0",
        routePlan: []
      )
      completion(.init(jupiterQuote: jupiterQuote), .mockJupiterFees, nil, "")
    }
    let store = SwapTokenStore(
      keyringService: keyringService,
      blockchainRegistry: blockchainRegistry,
      rpcService: rpcService,
      swapService: swapService,
      txService: txService,
      walletService: walletService,
      ethTxManagerProxy: ethTxManagerProxy,
      solTxManagerProxy: solTxManagerProxy,
      userAssetManager: mockAssetManager,
      prefilledToken: nil
    )

    let buyAmountExpectation = expectation(description: "buyAmountExpectation")
    store.$buyAmount
      .dropFirst()
      .collect(3)
      .sink { buyAmounts in
        defer { buyAmountExpectation.fulfill() }
        guard let buyAmount = buyAmounts.last else {
          XCTFail("Expected multiple buyAmount assignments.")
          return
        }
        XCTAssertFalse(buyAmount.isEmpty)
        XCTAssertEqual(buyAmount, "2.5000")
      }
      .store(in: &cancellables)

    let currentSwapQuoteInfoExpectation = expectation(
      description: "currentSwapQuoteInfoExpectation"
    )
    store.$currentSwapQuoteInfo
      .dropFirst()  // initial state
      .collect(2)  // fetchPriceQuote (nil), fetchSolPriceQuote (populated)
      .sink { currentSwapQuoteInfos in
        guard let currentSwapQuoteInfo = currentSwapQuoteInfos.last else {
          XCTFail("Expected multiple assignments.")
          return
        }
        XCTAssertNotNil(currentSwapQuoteInfo?.swapQuote)
        XCTAssertNotNil(currentSwapQuoteInfo?.swapFees)
        currentSwapQuoteInfoExpectation.fulfill()
      }
      .store(in: &cancellables)

    XCTAssertTrue(store.buyAmount.isEmpty)
    // non-empty assignment to `sellAmount` calls fetchPriceQuote
    store.setUpTest(
      fromAccount: .mockSolAccount,
      selectedFromToken: .mockSolToken,
      selectedToToken: .mockSpdToken,
      sellAmount: "0.01"
    )
    waitForExpectations(timeout: 2) { error in
      XCTAssertNil(error)
      // Verify fees
      XCTAssertEqual(store.braveFeeForDisplay, "0.85%")
    }
  }

  /// Test change to `sellAmount` (from value) will fetch Jupiter price quote and display insufficient liquidity (when returned by Jupiter price quote)
  func testFetchJupiterPriceQuoteInsufficientLiquidity() {
    let (
      keyringService, blockchainRegistry, rpcService, swapService, txService, walletService,
      ethTxManagerProxy, solTxManagerProxy, mockAssetManager
    ) = setupServices(
      network: .mockSolana,
      coin: .sol
    )
    swapService._quote = { _, completion in
      let errorUnion: BraveWallet.SwapErrorUnion = .init(
        jupiterError: .init(
          statusCode: "",
          error: "",
          message: "",
          isInsufficientLiquidity: true
        )
      )
      completion(nil, nil, errorUnion, "")
    }
    let store = SwapTokenStore(
      keyringService: keyringService,
      blockchainRegistry: blockchainRegistry,
      rpcService: rpcService,
      swapService: swapService,
      txService: txService,
      walletService: walletService,
      ethTxManagerProxy: ethTxManagerProxy,
      solTxManagerProxy: solTxManagerProxy,
      userAssetManager: mockAssetManager,
      prefilledToken: nil
    )

    let stateExpectation = expectation(description: "stateExpectation")
    store.$state
      .dropFirst()
      .collect(5)  // sellAmount, buyAmount didSet to `.idle`
      .sink { states in
        defer { stateExpectation.fulfill() }
        guard let state = states.last else {
          XCTFail("Expected multiple state updates.")
          return
        }
        XCTAssertEqual(state, .error(Strings.Wallet.insufficientLiquidity))
      }
      .store(in: &cancellables)

    XCTAssertNotEqual(store.state, .error(Strings.Wallet.insufficientLiquidity))
    // non-empty assignment to `sellAmount` calls fetchPriceQuote
    store.setUpTest(
      fromAccount: .mockSolAccount,
      selectedFromToken: .mockSolToken,
      selectedToToken: .mockSpdToken,
      sellAmount: "0.01"
    )
    waitForExpectations(timeout: 2) { error in
      XCTAssertNil(error)
    }
  }

  /// Test change to `sellAmount` (from value) will fetch LiFi price quote and assign to `buyAmount`
  func testFetchLiFiPriceQuoteSell() {
    let (
      keyringService, blockchainRegistry, rpcService, swapService, txService, walletService,
      ethTxManagerProxy, solTxManagerProxy, mockAssetManager
    ) = setupServices()
    let lifiQuote: BraveWallet.LiFiQuote = .mockOneETHtoUSDCQuote
    swapService._quote = { _, completion in
      completion(.init(lifiQuote: lifiQuote), .mockEthFees, nil, "")
    }
    let store = SwapTokenStore(
      keyringService: keyringService,
      blockchainRegistry: blockchainRegistry,
      rpcService: rpcService,
      swapService: swapService,
      txService: txService,
      walletService: walletService,
      ethTxManagerProxy: ethTxManagerProxy,
      solTxManagerProxy: solTxManagerProxy,
      userAssetManager: mockAssetManager,
      prefilledToken: nil
    )

    let buyAmountExpectation = expectation(description: "buyAmountExpectation")
    store.$buyAmount
      .dropFirst()
      .collect(3)
      .sink { buyAmounts in
        defer { buyAmountExpectation.fulfill() }
        guard let buyAmount = buyAmounts.last else {
          XCTFail("Expected multiple buyAmount assignments.")
          return
        }
        XCTAssertFalse(buyAmount.isEmpty)
        XCTAssertEqual(buyAmount, "3537.7229")
      }
      .store(in: &cancellables)

    XCTAssertTrue(store.buyAmount.isEmpty)
    // non-empty assignment to `sellAmount` calls fetchPriceQuote
    store.setUpTest(sellAmount: "1")
    waitForExpectations(timeout: 2) { error in
      XCTAssertNil(error)
      // Verify fees
      XCTAssertEqual(store.braveFeeForDisplay, "0.875%")
    }
  }

  /// Test change to `sellAmount` (from value) will fetch LiFi price quote and display insufficient liquidity (when returned by LiFi price quote)
  func testFetchLiFiPriceQuoteInsufficientLiquidity() {
    let (
      keyringService, blockchainRegistry, rpcService, swapService, txService, walletService,
      ethTxManagerProxy, solTxManagerProxy, mockAssetManager
    ) = setupServices(
      network: .mockMainnet,
      coin: .eth
    )
    swapService._quote = { _, completion in
      let errorUnion: BraveWallet.SwapErrorUnion = .init(
        lifiError: .init(
          message: "",
          code: .notFoundError
        )
      )
      completion(nil, nil, errorUnion, "")
    }
    let store = SwapTokenStore(
      keyringService: keyringService,
      blockchainRegistry: blockchainRegistry,
      rpcService: rpcService,
      swapService: swapService,
      txService: txService,
      walletService: walletService,
      ethTxManagerProxy: ethTxManagerProxy,
      solTxManagerProxy: solTxManagerProxy,
      userAssetManager: mockAssetManager,
      prefilledToken: nil
    )

    let stateExpectation = expectation(description: "stateExpectation")
    store.$state
      .dropFirst()
      .collect(5)  // sellAmount, buyAmount didSet to `.idle`
      .sink { states in
        defer { stateExpectation.fulfill() }
        guard let state = states.last else {
          XCTFail("Expected multiple state updates.")
          return
        }
        XCTAssertEqual(state, .error(Strings.Wallet.insufficientLiquidity))
      }
      .store(in: &cancellables)

    XCTAssertNotEqual(store.state, .error(Strings.Wallet.insufficientLiquidity))
    // non-empty assignment to `sellAmount` calls fetchPriceQuote
    store.setUpTest(
      fromAccount: .mockEthAccount,
      selectedFromToken: .previewToken,
      selectedToToken: .mockUSDCToken,
      sellAmount: "0.01"
    )
    waitForExpectations(timeout: 2) { error in
      XCTAssertNil(error)
    }
  }

  /// Test creating ERC20 approve transaction
  @MainActor func testSwapERC20ApproveTransaction() async {
    let (
      keyringService, blockchainRegistry, rpcService, swapService, txService, walletService,
      ethTxManagerProxy, solTxManagerProxy, mockAssetManager
    ) = setupServices()
    var submittedParams: BraveWallet.NewEvmTransactionParams?
    txService._addUnapprovedEvmTransaction = { params, completion in
      submittedParams = params
      completion(true, "tx-meta-id", "")
    }
    let store = SwapTokenStore(
      keyringService: keyringService,
      blockchainRegistry: blockchainRegistry,
      rpcService: rpcService,
      swapService: swapService,
      txService: txService,
      walletService: walletService,
      ethTxManagerProxy: ethTxManagerProxy,
      solTxManagerProxy: solTxManagerProxy,
      userAssetManager: mockAssetManager,
      prefilledToken: nil
    )
    store.setUpTest()
    store.state = .lowAllowance("test-spender-address")

    let success = await store.createSwapTransaction()
    XCTAssertTrue(success, "Expected to successfully create transaction")
    XCTAssertFalse(store.isMakingTx)
    XCTAssertNotNil(submittedParams)
  }

  /// Test creating a eth swap transaction using ZeroEx quote
  @MainActor func testZeroExSwapETHTransaction() async {
    let (
      keyringService, blockchainRegistry, rpcService, swapService, txService, walletService,
      ethTxManagerProxy, solTxManagerProxy, mockAssetManager
    ) = setupServices()
    var submittedParams: BraveWallet.NewEvmTransactionParams?
    txService._addUnapprovedEvmTransaction = { params, completion in
      submittedParams = params
      completion(true, "tx-meta-id", "")
    }
    let store = SwapTokenStore(
      keyringService: keyringService,
      blockchainRegistry: blockchainRegistry,
      rpcService: rpcService,
      swapService: swapService,
      txService: txService,
      walletService: walletService,
      ethTxManagerProxy: ethTxManagerProxy,
      solTxManagerProxy: solTxManagerProxy,
      userAssetManager: mockAssetManager,
      prefilledToken: nil
    )
    store.setUpTest(
      currentSwapQuoteInfo: .init(
        base: .perSellAsset,
        swapQuote: .init(zeroExQuote: .mockOneETHToBATQuote),
        swapFees: nil
      )
    )
    store.state = .swap

    let success = await store.createSwapTransaction()
    XCTAssertTrue(success, "Expected to successfully create transaction")
    XCTAssertFalse(store.isMakingTx)
    XCTAssertNotNil(submittedParams)
  }

  /// Test creating a eth swap transaction using LiFi quote
  @MainActor func testLiFiSwapETHTransaction() async {
    let (
      keyringService, blockchainRegistry, rpcService, swapService, txService, walletService,
      ethTxManagerProxy, solTxManagerProxy, mockAssetManager
    ) = setupServices()
    var submittedParams: BraveWallet.NewEvmTransactionParams?
    txService._addUnapprovedEvmTransaction = { params, completion in
      submittedParams = params
      completion(true, "tx-meta-id", "")
    }
    let store = SwapTokenStore(
      keyringService: keyringService,
      blockchainRegistry: blockchainRegistry,
      rpcService: rpcService,
      swapService: swapService,
      txService: txService,
      walletService: walletService,
      ethTxManagerProxy: ethTxManagerProxy,
      solTxManagerProxy: solTxManagerProxy,
      userAssetManager: mockAssetManager,
      prefilledToken: nil
    )
    store.setUpTest(
      currentSwapQuoteInfo: .init(
        base: .perSellAsset,
        swapQuote: .init(lifiQuote: .mockOneETHtoUSDCQuote),
        swapFees: nil
      )
    )
    store.state = .swap

    let success = await store.createSwapTransaction()
    XCTAssertTrue(success, "Expected to successfully create transaction")
    XCTAssertFalse(store.isMakingTx)
    XCTAssertNotNil(submittedParams)
  }

  /// Test creating a sol swap transaction
  @MainActor func testSwapSolSwapTransaction() async {
    let jupiterQuote: BraveWallet.JupiterQuote = .init(
      inputMint: BraveWallet.BlockchainToken.mockSolToken.contractAddress,
      inAmount: "3000000000",
      outputMint: BraveWallet.BlockchainToken.mockSpdToken.contractAddress,
      outAmount: "2500000",  // 2.5 SPD (6 decimals)
      otherAmountThreshold: "2500000",  // 2.5 SPD (6 decimals)
      swapMode: "",
      slippageBps: "50",  // 0.5%
      platformFee: nil,
      priceImpactPct: "0",
      routePlan: []
    )
    let (
      keyringService, blockchainRegistry, rpcService, swapService, txService, walletService,
      ethTxManagerProxy, solTxManagerProxy, mockAssetManager
    ) = setupServices(
      network: .mockSolana,
      coin: .sol
    )
    swapService._transaction = { _, completion in
      completion(.init(jupiterTransaction: "1"), nil, "")
    }
    solTxManagerProxy._makeTxDataFromBase64EncodedTransaction = { _, _, _, completion in
      completion(.init(), .success, "")
    }
    var submittedTxData: BraveWallet.TxDataUnion?
    txService._addUnapprovedTransaction = { txData, _, _, completion in
      submittedTxData = txData
      completion(true, "tx-meta-id", "")
    }
    let store = SwapTokenStore(
      keyringService: keyringService,
      blockchainRegistry: blockchainRegistry,
      rpcService: rpcService,
      swapService: swapService,
      txService: txService,
      walletService: walletService,
      ethTxManagerProxy: ethTxManagerProxy,
      solTxManagerProxy: solTxManagerProxy,
      userAssetManager: mockAssetManager,
      prefilledToken: nil
    )
    store.setUpTest(
      fromAccount: .mockSolAccount,
      selectedFromToken: .mockSolToken,
      selectedToToken: .mockSpdToken,
      sellAmount: "0.01",
      currentSwapQuoteInfo: .init(
        base: .perSellAsset,
        swapQuote: .init(jupiterQuote: jupiterQuote),
        swapFees: .init(
          feeParam: "",
          feePct: "",
          discountPct: "",
          effectiveFeePct: "",
          discountCode: .none
        )
      )
    )
    store.state = .swap

    let success = await store.createSwapTransaction()
    XCTAssertTrue(success, "Expected to successfully create transaction")
    XCTAssertFalse(store.isMakingTx)
    XCTAssertNotNil(submittedTxData?.solanaTxData)
  }

  func testSwapFullBalanceNoRounding() {
    let formatter = WalletAmountFormatter(decimalFormatStyle: .decimals(precision: 18))
    let mockBalance = "47.156499657504857477"
    let mockBalanceWei = formatter.weiString(from: mockBalance, radix: .hex, decimals: 18) ?? ""

    let rpcService = MockJsonRpcService()
    rpcService._chainIdForOrigin = { $2(BraveWallet.NetworkInfo.mockSepolia.chainId) }
    rpcService._network = { $2(BraveWallet.NetworkInfo.mockSepolia) }
    rpcService._balance = { _, _, _, completion in
      completion(mockBalanceWei, .success, "")
    }
    let solTxManagerProxy = BraveWallet.TestSolanaTxManagerProxy()

    let store = SwapTokenStore(
      keyringService: MockKeyringService(),
      blockchainRegistry: MockBlockchainRegistry(),
      rpcService: rpcService,
      swapService: MockSwapService(),
      txService: MockTxService(),
      walletService: MockBraveWalletService(),
      ethTxManagerProxy: MockEthTxManagerProxy(),
      solTxManagerProxy: solTxManagerProxy,
      userAssetManager: TestableWalletUserAssetManager(),
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
