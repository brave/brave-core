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
    let batToken: BraveWallet.BlockchainToken = .init(contractAddress: "", name: "Brave BAT", logo: "", isErc20: true, isErc721: false, isErc1155: false, isNft: false, isSpam: false, symbol: "BAT", decimals: 18, visible: false, tokenId: "", coingeckoId: "", chainId: BraveWallet.MainnetChainId, coin: .eth)
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
    
    let (keyringService, blockchainRegistry, rpcService, swapService, txService, walletService, ethTxManagerProxy, solTxManagerProxy, mockAssetManager) = setupServices()
    var selectedNetwork: BraveWallet.NetworkInfo = .mockMainnet
    
    rpcService._network = { coin, _, completion in
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
    rpcService._setNetwork = { chainId, coin, origin, completion in
      XCTAssertEqual(chainId, BraveWallet.SolanaMainnet) // verify network switched to SolanaMainnet
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
    XCTAssertNil(store.selectedFromToken) // `prefilledToken` not set until validated in `prepare()`
    XCTAssertNil(store.selectedToToken)
    let testAccountInfo: BraveWallet.AccountInfo = .init()
    store.prepare(with: testAccountInfo) {
      defer { ex.fulfill() }
      XCTAssertEqual(store.selectedFromToken?.symbol.lowercased(), prefilledToken.symbol.lowercased())
      XCTAssertNotNil(store.selectedToToken)
      XCTAssertNotEqual(store.selectedToToken?.symbol.lowercased(), prefilledToken.symbol.lowercased())
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
    
    rpcService.setNetwork(BraveWallet.PolygonMainnetChainId, coin: .eth, origin: nil) { success in
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
    let daiToken: BraveWallet.BlockchainToken = .init(contractAddress: "", name: "DAI Stablecoin", logo: "", isErc20: true, isErc721: false, isErc1155: false, isNft: false, isSpam: false, symbol: "DAI", decimals: 18, visible: false, tokenId: "", coingeckoId: "", chainId: BraveWallet.PolygonMainnetChainId, coin: .eth)
    let batToken: BraveWallet.BlockchainToken = .init(contractAddress: "0x0d8775f648430679a709e98d2b0cb6250d2887ef", name: "Basic Attention Token", logo: "", isErc20: true, isErc721: false, isErc1155: false, isNft: false, isSpam: false, symbol: "BAT", decimals: 18, visible: true, tokenId: "", coingeckoId: "", chainId: BraveWallet.PolygonMainnetChainId, coin: .eth)
    let rpcService = BraveWallet.TestJsonRpcService()
    rpcService._addObserver = { _ in }
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

  private func setupServices(
    network: BraveWallet.NetworkInfo = .mockMainnet,
    coin: BraveWallet.CoinType = .eth
  ) -> (BraveWallet.TestKeyringService, BraveWallet.TestBlockchainRegistry, BraveWallet.TestJsonRpcService, BraveWallet.TestSwapService, BraveWallet.TestTxService, BraveWallet.TestBraveWalletService, BraveWallet.TestEthTxManagerProxy, BraveWallet.TestSolanaTxManagerProxy, WalletUserAssetManagerType) {
    let keyringService = BraveWallet.TestKeyringService()
    keyringService._addObserver = { _ in }
    let blockchainRegistry = BraveWallet.TestBlockchainRegistry()
    blockchainRegistry._allTokens = { $2([.previewToken, .previewDaiToken]) }
    let rpcService = BraveWallet.TestJsonRpcService()
    rpcService._addObserver = { _ in }
    rpcService._network = { $2(network) }
    rpcService._balance = { _, _, _, completion in
      // return fake sufficient ETH balance `0x13e25e19dc20ba7` is about 0.0896 ETH
      completion("0x13e25e19dc20ba7", .success, "")
    }
    rpcService._erc20TokenBalance = { _, _, _, completion in
      // return fake sufficient ETH balance `0x13e25e19dc20ba7` is about 0.0896 ETH
      completion("0x13e25e19dc20ba7", .success, "")
    }
    rpcService._solanaBalance = { _, _, completion in
      completion(0, .internalError, "")
    }
    rpcService._splTokenAccountBalance = { _, _, _, completion in
      completion("", 0, "", .internalError, "")
    }
    let swapService = BraveWallet.TestSwapService()
    swapService._quote = { _, completion in
      if coin == .eth {
        completion(.init(zeroExQuote: .init()), nil, "")
      } else if coin == .sol {
        completion(.init(jupiterQuote: .init()), nil, "")
      } else {
        XCTFail("Coin type is not supported for swap")
      }
    }
    swapService._transaction = { _, completion in
      if coin == .eth {
        completion(.init(zeroExTransaction: .init()), nil, "")
      } else if coin == .sol {
        completion(.init(jupiterTransaction: .init()), nil, "")
      } else {
        XCTFail("Coin type is not supported for swap")
      }
    }
    swapService._braveFee = { params, completion in
      completion(nil, "")
    }
    let txService = BraveWallet.TestTxService()
    txService._addUnapprovedTransaction = { $3(true, "tx-meta-id", "") }
    let walletService = BraveWallet.TestBraveWalletService()
    let mockAssetManager = TestableWalletUserAssetManager()
    mockAssetManager._getAllUserAssetsInNetworkAssets = { _, _ in
      network.coin == .eth ? [NetworkAssets(network: .mockMainnet, tokens: [.previewToken, .previewDaiToken], sortOrder: 0)] : [NetworkAssets(network: .mockSolana, tokens: [.mockSolToken, .mockSpdToken], sortOrder: 0)]
    }
    let ethTxManagerProxy = BraveWallet.TestEthTxManagerProxy()
    ethTxManagerProxy._makeErc20ApproveData = { $2(true, []) }
    ethTxManagerProxy._gasEstimation1559 = { $1(.init()) }
    let solTxManagerProxy = BraveWallet.TestSolanaTxManagerProxy()
    return (keyringService, blockchainRegistry, rpcService, swapService, txService, walletService, ethTxManagerProxy, solTxManagerProxy, mockAssetManager)
  }
  
  /// Test change to `sellAmount` (from value) will fetch price quote and assign to `buyAmount`
  func testFetchPriceQuoteSell() {
    let (keyringService, blockchainRegistry, rpcService, swapService, txService, walletService, ethTxManagerProxy, solTxManagerProxy, mockAssetManager) = setupServices()
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
      fees: .init(zeroExFee: .init(
        feeType: "",
        feeToken: "",
        feeAmount: "",
        billingType: ""
      ))
    )
    swapService._quote = { _, completion in
      completion(.init(zeroExQuote: zeroExQuote), nil, "")
    }
    swapService._braveFee = { params, completion in
      let feeResponse = BraveWallet.BraveSwapFeeResponse(
        feeParam: "0.00875",
        protocolFeePct: "0.15",
        braveFeePct: "0.875",
        discountOnBraveFeePct: "0",
        effectiveFeePct: "0.875",
        discountCode: .none,
        hasBraveFee: true
      )
      completion(feeResponse, "")
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
    waitForExpectations(timeout: 1) { error in
      XCTAssertNil(error)
      // Verify fees
      XCTAssertEqual(store.braveFeeForDisplay, "0.875%")
      XCTAssertEqual(store.protocolFeeForDisplay, "0.15%")
    }
  }
  
  /// Test change to `buyAmount` (to value) will fetch price quote and assign to `buyAmount`
  func testFetchPriceQuoteBuy() {
    let (keyringService, blockchainRegistry, rpcService, swapService, txService, walletService, ethTxManagerProxy, solTxManagerProxy, mockAssetManager) = setupServices()
    let zeroExQuote: BraveWallet.ZeroExQuote = .init()
    zeroExQuote.sellAmount = "3000000000000000000"
    swapService._quote = { _, completion in
      completion(.init(zeroExQuote: zeroExQuote), nil, "")
    }
    swapService._braveFee = { params, completion in
      let feeResponse = BraveWallet.BraveSwapFeeResponse(
        feeParam: "0.00875",
        protocolFeePct: "0.15",
        braveFeePct: "0.875",
        discountOnBraveFeePct: "0",
        effectiveFeePct: "0.875",
        discountCode: .none,
        hasBraveFee: true
      )
      completion(feeResponse, "")
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
    
    let braveFeeExpectation = expectation(description: "braveFeeExpectation")
    store.$braveFee
      .dropFirst()
      .collect(3)
      .sink { _ in
        braveFeeExpectation.fulfill()
      }
      .store(in: &cancellables)

    XCTAssertTrue(store.sellAmount.isEmpty)
    // calls fetchPriceQuote
    store.setUpTest(sellAmount: nil, buyAmount: "0.01")
    waitForExpectations(timeout: 1) { error in
      XCTAssertNil(error)
      // Verify fees
      XCTAssertEqual(store.braveFeeForDisplay, "0.875%")
      XCTAssertNil(store.protocolFeeForDisplay)
    }
  }
  
  /// Test change to `sellAmount` (from value) will fetch price quote and assign to `buyAmount` on Solana Mainnet
  func testSolanaFetchPriceQuoteSell() {
    let (keyringService, blockchainRegistry, rpcService, swapService, txService, walletService, ethTxManagerProxy, solTxManagerProxy, mockAssetManager) = setupServices(
      network: .mockSolana,
      coin: .sol
    )
    swapService._quote = { swapQuoteParams, completion in
      // verify 0.005 is converted to 0.5
      XCTAssertEqual(swapQuoteParams.slippagePercentage, "0.5")
      let jupiterQuote: BraveWallet.JupiterQuote = .init(
        inputMint: BraveWallet.BlockchainToken.mockSolToken.contractAddress,
        inAmount: "10000000", // 0.01 SOL (9 decimals)
        outputMint: BraveWallet.BlockchainToken.mockSpdToken.contractAddress,
        outAmount: "2500000", // 2.5 SPD (6 decimals)
        otherAmountThreshold: "2500000", // 2.5 SPD (6 decimals)
        swapMode: "",
        slippageBps: "50", // 0.5%
        platformFee: nil,
        priceImpactPct: "0",
        routePlan: []
      )
      completion(.init(jupiterQuote: jupiterQuote), nil, "")
    }
    swapService._braveFee = { params, completion in
      let feeResponse = BraveWallet.BraveSwapFeeResponse(
        feeParam: "85",
        protocolFeePct: "0",
        braveFeePct: "0.85",
        discountOnBraveFeePct: "0",
        effectiveFeePct: "0.85",
        discountCode: .none,
        hasBraveFee: true
      )
      completion(feeResponse, "")
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
    
    let braveFeeExpectation = expectation(description: "braveFeeExpectation")
    store.$braveFee
      .dropFirst()
      .collect(3)
      .sink { _ in
        braveFeeExpectation.fulfill()
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
    waitForExpectations(timeout: 1) { error in
      XCTAssertNil(error)
      // Verify fees
      XCTAssertEqual(store.braveFeeForDisplay, "0.85%")
      XCTAssertNil(store.protocolFeeForDisplay)
    }
  }
  
  /// Test change to `sellAmount` (from value) will fetch price quote and display insufficient liquidity (when returned by Jupiter price quote)
  func testSolanaFetchPriceQuoteInsufficientLiquidity() {
    let (keyringService, blockchainRegistry, rpcService, swapService, txService, walletService, ethTxManagerProxy, solTxManagerProxy, mockAssetManager) = setupServices(
      network: .mockSolana,
      coin: .sol
    )
    swapService._quote = { _, completion in
      let errorUnion: BraveWallet.SwapErrorUnion = .init(jupiterError: .init(
        statusCode: "",
        error: "",
        message: "",
        isInsufficientLiquidity: true)
      )
      completion(nil, errorUnion, "")
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
      .collect(5) // sellAmount, buyAmount didSet to `.idle`
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
    waitForExpectations(timeout: 1) { error in
      XCTAssertNil(error)
    }
  }

  /// Test creating ERC20 approve transaction on EIP1559 network
  @MainActor func testSwapERC20EIP1559Transaction() async {
    let (keyringService, blockchainRegistry, rpcService, swapService, txService, walletService, ethTxManagerProxy, solTxManagerProxy, mockAssetManager) = setupServices()
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
    store.setUpTest()
    store.state = .lowAllowance("test-spender-address")
    let success = await store.createSwapTransaction()
    XCTAssertTrue(success, "Expected to successfully create transaction")
    XCTAssertFalse(store.isMakingTx)
    XCTAssertNotNil(submittedTxData?.ethTxData1559)
    XCTAssertNil(submittedTxData?.ethTxData)
  }

  /// Test creating ERC20 approve transaction on non-EIP1559 network
  @MainActor func testSwapERC20Transaction() async {
    // Celo Mainnet / `.mockCelo` is not EIP1559
    let (keyringService, blockchainRegistry, rpcService, swapService, txService, walletService, ethTxManagerProxy, solTxManagerProxy, mockAssetManager) = setupServices(network: .mockCelo)
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
    store.setUpTest()
    store.state = .lowAllowance("test-spender-address")
    
    let success = await store.createSwapTransaction()
    XCTAssertTrue(success, "Expected to successfully create transaction")
    XCTAssertFalse(store.isMakingTx)
    XCTAssertNotNil(submittedTxData?.ethTxData)
    XCTAssertNil(submittedTxData?.ethTxData1559)
  }

  /// Test creating a eth swap transaction on EIP1559 network
  @MainActor func testSwapETHSwapEIP1559Transaction() async {
    let (keyringService, blockchainRegistry, rpcService, swapService, txService, walletService, ethTxManagerProxy, solTxManagerProxy, mockAssetManager) = setupServices()
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
    store.setUpTest()
    store.state = .swap
    
    let success = await store.createSwapTransaction()
    XCTAssertTrue(success, "Expected to successfully create transaction")
    XCTAssertFalse(store.isMakingTx)
    XCTAssertNotNil(submittedTxData?.ethTxData1559)
    XCTAssertNil(submittedTxData?.ethTxData)
  }

  /// Test creating a eth swap transaction on non-EIP1559 network
  @MainActor func testSwapETHSwapTransaction() async {
    // Celo Mainnet / `.mockCelo` is not EIP1559
    let (keyringService, blockchainRegistry, rpcService, swapService, txService, walletService, ethTxManagerProxy, solTxManagerProxy, mockAssetManager) = setupServices(network: .mockCelo)
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
    store.setUpTest()
    store.state = .swap
    
    let success = await store.createSwapTransaction()
    XCTAssertTrue(success, "Expected to successfully create transaction")
    XCTAssertFalse(store.isMakingTx)
    XCTAssertNotNil(submittedTxData?.ethTxData)
    XCTAssertNil(submittedTxData?.ethTxData1559)
  }

  /// Test creating a sol swap transaction
  @MainActor func testSwapSolSwapTransaction() async {
    let jupiterQuote: BraveWallet.JupiterQuote = .init(
      inputMint: BraveWallet.BlockchainToken.mockSolToken.contractAddress,
      inAmount: "3000000000",
      outputMint: BraveWallet.BlockchainToken.mockSpdToken.contractAddress,
      outAmount: "2500000", // 2.5 SPD (6 decimals)
      otherAmountThreshold: "2500000", // 2.5 SPD (6 decimals)
      swapMode: "",
      slippageBps: "50", // 0.5%
      platformFee: nil,
      priceImpactPct: "0",
      routePlan: []
    )
    let (keyringService, blockchainRegistry, rpcService, swapService, txService, walletService, ethTxManagerProxy, solTxManagerProxy, mockAssetManager) = setupServices(
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
      jupiterQuote: jupiterQuote
    )
    store.state = .swap
    
    let success = await store.createSwapTransaction()
    XCTAssertTrue(success, "Expected to successfully create transaction")
    XCTAssertFalse(store.isMakingTx)
    XCTAssertNotNil(submittedTxData?.solanaTxData)
  }
  
  func testSwapFullBalanceNoRounding() {
    let formatter = WeiFormatter(decimalFormatStyle: .decimals(precision: 18))
    let mockBalance = "47.156499657504857477"
    let mockBalanceWei = formatter.weiString(from: mockBalance, radix: .hex, decimals: 18) ?? ""
    
    let rpcService = BraveWallet.TestJsonRpcService()
    rpcService._chainIdForOrigin = { $2(BraveWallet.NetworkInfo.mockGoerli.chainId) }
    rpcService._network = { $2(BraveWallet.NetworkInfo.mockGoerli)}
    rpcService._balance = { _, _, _, completion in
      completion(mockBalanceWei, .success, "")
    }
    rpcService._addObserver = { _ in }
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
