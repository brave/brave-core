// Copyright 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import XCTest
import Combine
import BraveCore
import BigNumber
@testable import BraveWallet

class SendTokenStoreTests: XCTestCase {
  private var cancellables: Set<AnyCancellable> = []
  private let batSymbol = "BAT"

  func testPrefilledToken() {
    let solTxManagerProxy = BraveWallet.TestSolanaTxManagerProxy()
    
    var store = SendTokenStore(
      keyringService: MockKeyringService(),
      rpcService: MockJsonRpcService(),
      walletService: MockBraveWalletService(),
      txService: MockTxService(),
      blockchainRegistry: MockBlockchainRegistry(),
      ethTxManagerProxy: MockEthTxManagerProxy(),
      solTxManagerProxy: solTxManagerProxy,
      prefilledToken: nil
    )
    XCTAssertNil(store.selectedSendToken)

    store = SendTokenStore(
      keyringService: MockKeyringService(),
      rpcService: MockJsonRpcService(),
      walletService: MockBraveWalletService(),
      txService: MockTxService(),
      blockchainRegistry: MockBlockchainRegistry(),
      ethTxManagerProxy: MockEthTxManagerProxy(),
      solTxManagerProxy: solTxManagerProxy,
      prefilledToken: .previewToken
    )
    XCTAssertEqual(store.selectedSendToken?.symbol.lowercased(), BraveWallet.BlockchainToken.previewToken.symbol.lowercased())
  }

  func testFetchAssets() {
    let rpcService = BraveWallet.TestJsonRpcService()
    rpcService._network = { $1(.mockRopsten) }
    rpcService._addObserver = { _ in }
    
    let walletService = BraveWallet.TestBraveWalletService()
    walletService._selectedCoin = { $0(.eth) }
    walletService._userAssets = { $2([.previewToken]) }
    
    let solTxManagerProxy = BraveWallet.TestSolanaTxManagerProxy()
    
    let store = SendTokenStore(
      keyringService: MockKeyringService(),
      rpcService: rpcService,
      walletService: walletService,
      txService: MockTxService(),
      blockchainRegistry: MockBlockchainRegistry(),
      ethTxManagerProxy: MockEthTxManagerProxy(),
      solTxManagerProxy: solTxManagerProxy,
      prefilledToken: nil
    )
    let ex = expectation(description: "fetch-assets")
    XCTAssertNil(store.selectedSendToken)  // Initial state
    store.$selectedSendToken.dropFirst().sink { token in
      defer { ex.fulfill() }
      guard let token = token else {
        XCTFail("Token was nil")
        return
      }
      rpcService.network(.eth) { network in
        XCTAssertEqual(token.symbol, network.symbol)
      }
    }.store(in: &cancellables)
    store.fetchAssets()
    waitForExpectations(timeout: 3) { error in
      XCTAssertNil(error)
    }
  }

  func testMakeSendETHEIP1559Transaction() {
    let walletService = BraveWallet.TestBraveWalletService()
    walletService._selectedCoin = { $0(.eth) }
    walletService._userAssets = { $2([.previewToken]) }
    
    let solTxManagerProxy = BraveWallet.TestSolanaTxManagerProxy()
    
    let store = SendTokenStore(
      keyringService: MockKeyringService(),
      rpcService: MockJsonRpcService(),
      walletService: walletService,
      txService: MockTxService(),
      blockchainRegistry: MockBlockchainRegistry(),
      ethTxManagerProxy: MockEthTxManagerProxy(),
      solTxManagerProxy: solTxManagerProxy,
      prefilledToken: .previewToken
    )
    store.setUpTest()
    let ex = expectation(description: "send-eth-eip1559-transaction")
    store.sendToken(amount: "0.01") { success, _ in
      defer { ex.fulfill() }
      XCTAssertTrue(success)
    }
    waitForExpectations(timeout: 3) { error in
      XCTAssertNil(error)
    }
  }

  func testMakeSendETHTransaction() {
    let rpcService = BraveWallet.TestJsonRpcService()
    rpcService._network = { $1(.mockRopsten) }
    rpcService._addObserver = { _ in }
    
    let walletService = BraveWallet.TestBraveWalletService()
    walletService._selectedCoin = { $0(.eth) }
    walletService._userAssets = { $2([.previewToken]) }
    
    let solTxManagerProxy = BraveWallet.TestSolanaTxManagerProxy()
    
    let store = SendTokenStore(
      keyringService: MockKeyringService(),
      rpcService: rpcService,
      walletService: walletService,
      txService: MockTxService(),
      blockchainRegistry: MockBlockchainRegistry(),
      ethTxManagerProxy: MockEthTxManagerProxy(),
      solTxManagerProxy: solTxManagerProxy,
      prefilledToken: .previewToken
    )
    store.setUpTest()

    let ex = expectation(description: "send-eth-transaction")
    store.sendToken(amount: "0.01") { success, _ in
      defer { ex.fulfill() }
      XCTAssertTrue(success)
    }
    waitForExpectations(timeout: 3) { error in
      XCTAssertNil(error)
    }
  }

  func testMakeSendERC20EIP1559Transaction() {
    let walletService = BraveWallet.TestBraveWalletService()
    walletService._selectedCoin = { $0(.eth) }
    walletService._userAssets = { $2([.previewToken]) }
    
    let solTxManagerProxy = BraveWallet.TestSolanaTxManagerProxy()
    
    let store = SendTokenStore(
      keyringService: MockKeyringService(),
      rpcService: MockJsonRpcService(),
      walletService: walletService,
      txService: MockTxService(),
      blockchainRegistry: MockBlockchainRegistry(),
      ethTxManagerProxy: MockEthTxManagerProxy(),
      solTxManagerProxy: solTxManagerProxy,
      prefilledToken: nil
    )
    let token: BraveWallet.BlockchainToken = .init(contractAddress: "0x0d8775f648430679a709e98d2b0cb6250d2887ef", name: "Basic Attention Token", logo: "", isErc20: true, isErc721: false, symbol: batSymbol, decimals: 18, visible: true, tokenId: "", coingeckoId: "", chainId: "", coin: .eth)
    store.selectedSendToken = token
    store.setUpTest()

    let ex = expectation(description: "send-bat-eip1559-transaction")
    store.sendToken(amount: "0.01") { success, _ in
      defer { ex.fulfill() }
      XCTAssertTrue(success)
    }
    waitForExpectations(timeout: 3) { error in
      XCTAssertNil(error)
    }
  }

  func testMakeSendERC20Transaction() {
    let rpcService = BraveWallet.TestJsonRpcService()
    rpcService._network = { $1(.mockRopsten) }
    rpcService._addObserver = { _ in }
    
    let walletService = BraveWallet.TestBraveWalletService()
    walletService._selectedCoin = { $0(.eth) }
    
    let solTxManagerProxy = BraveWallet.TestSolanaTxManagerProxy()
    
    let store = SendTokenStore(
      keyringService: MockKeyringService(),
      rpcService: rpcService,
      walletService: MockBraveWalletService(),
      txService: MockTxService(),
      blockchainRegistry: MockBlockchainRegistry(),
      ethTxManagerProxy: MockEthTxManagerProxy(),
      solTxManagerProxy: solTxManagerProxy,
      prefilledToken: .previewToken
    )
    store.setUpTest()

    let ex = expectation(description: "send-bat-transaction")
    store.sendToken(amount: "0.01") { success, _ in
      defer { ex.fulfill() }
      XCTAssertTrue(success)
    }
    waitForExpectations(timeout: 3) { error in
      XCTAssertNil(error)
    }
  }
  
  func testSendFullBalanceNoRounding() {
    let formatter = WeiFormatter(decimalFormatStyle: .decimals(precision: 18))
    let mockBalance = "47.156499657504857477"
    let mockBalanceWei = formatter.weiString(from: mockBalance, radix: .hex, decimals: 18) ?? ""
    
    let rpcService = BraveWallet.TestJsonRpcService()
    rpcService._chainId = { $1(BraveWallet.NetworkInfo.mockRopsten.chainId) }
    rpcService._network = { $1(BraveWallet.NetworkInfo.mockRopsten)}
    rpcService._balance = { _, _, _, completion in
      completion(mockBalanceWei, .success, "")
    }
    rpcService._addObserver = { _ in }
    
    let walletService = BraveWallet.TestBraveWalletService()
    walletService._userAssets = { $2([.previewToken]) }
    walletService._selectedCoin = { $0(BraveWallet.CoinType.eth) }
    
    let keyringService = BraveWallet.TestKeyringService()
    keyringService._selectedAccount = { $1("account-address") }
    keyringService._addObserver = { _ in }
    
    let solTxManagerProxy = BraveWallet.TestSolanaTxManagerProxy()
    
    let store = SendTokenStore(
      keyringService: keyringService,
      rpcService: rpcService,
      walletService: walletService,
      txService: MockTxService(),
      blockchainRegistry: MockBlockchainRegistry(),
      ethTxManagerProxy: MockEthTxManagerProxy(),
      solTxManagerProxy: solTxManagerProxy,
      prefilledToken: .previewToken
    )
    store.setUpTest()
    
    let fetchSelectedTokenBalanceEx = expectation(description: "fetchSelectedTokenBalance")
    store.$selectedSendTokenBalance
      .sink { balance in
        XCTAssertEqual(balance, BDouble(mockBalance)!)
        store.suggestedAmountTapped(.all)
        fetchSelectedTokenBalanceEx.fulfill()
      }
      .store(in: &cancellables)
    
    let sendFullBalanceEx = expectation(description: "sendFullBalance")
    store.$sendAmount
      .sink { amount in
        XCTAssertEqual("\(amount)", "\(mockBalance)")
        sendFullBalanceEx.fulfill()
      }
      .store(in: &cancellables)
    
    waitForExpectations(timeout: 3) { error in
      XCTAssertNil(error)
    }
  }
  
  func testSolSendSystemProgramTransfer() {
    let mockBalance = 47
    
    let rpcService = BraveWallet.TestJsonRpcService()
    rpcService._chainId = { $1(BraveWallet.NetworkInfo.mockSolana.chainId) }
    rpcService._network = { $1(BraveWallet.NetworkInfo.mockSolana)}
    rpcService._solanaBalance = { _, _ , completion in
      completion(UInt64(mockBalance), .success, "")
    }
    rpcService._addObserver = { _ in }
    
    let walletService = BraveWallet.TestBraveWalletService()
    walletService._userAssets = { $2([.mockSolToken]) }
    walletService._selectedCoin = { $0(BraveWallet.CoinType.sol) }
    
    let keyringService = BraveWallet.TestKeyringService()
    keyringService._selectedAccount = { $1("account-address") }
    keyringService._addObserver = { _ in }
    
    let solTxManagerProxy = BraveWallet.TestSolanaTxManagerProxy()
    solTxManagerProxy._makeSystemProgramTransferTxData = {_, _, _, completion in
      completion(.init(), .success, "")
    }
    
    let store = SendTokenStore(
      keyringService: keyringService,
      rpcService: rpcService,
      walletService: walletService,
      txService: MockTxService(),
      blockchainRegistry: MockBlockchainRegistry(),
      ethTxManagerProxy: MockEthTxManagerProxy(),
      solTxManagerProxy: solTxManagerProxy,
      prefilledToken: BraveWallet.NetworkInfo.mockSolana.nativeToken
    )
    
    let ex = expectation(description: "send-sol-transaction")
    store.sendToken(
      amount: "0.01"
    ) { success, errMsg in
      defer { ex.fulfill() }
      XCTAssertTrue(success)
    }
    
    waitForExpectations(timeout: 3) { error in
      XCTAssertNil(error)
    }
  }
  
  func testSolSendTokenProgramTransfer() {
    let rpcService = BraveWallet.TestJsonRpcService()
    rpcService._chainId = { $1(BraveWallet.NetworkInfo.mockSolana.chainId) }
    rpcService._network = { $1(BraveWallet.NetworkInfo.mockSolana)}
    rpcService._splTokenAccountBalance = {_, _, _, completion in
      completion("1000000", UInt8(6), "1", .success, "")
    }
    rpcService._addObserver = { _ in }
    
    let walletService = BraveWallet.TestBraveWalletService()
    walletService._userAssets = { $2([.mockSpdToken]) }
    walletService._selectedCoin = { $0(BraveWallet.CoinType.sol) }
    
    let keyringService = BraveWallet.TestKeyringService()
    keyringService._selectedAccount = { $1("account-address") }
    keyringService._addObserver = { _ in }
    
    let solTxManagerProxy = BraveWallet.TestSolanaTxManagerProxy()
    solTxManagerProxy._makeTokenProgramTransferTxData = { _, _, _, _, completion in
      completion(.init(), .success, "")
    }
    
    let store = SendTokenStore(
      keyringService: keyringService,
      rpcService: rpcService,
      walletService: walletService,
      txService: MockTxService(),
      blockchainRegistry: MockBlockchainRegistry(),
      ethTxManagerProxy: MockEthTxManagerProxy(),
      solTxManagerProxy: solTxManagerProxy,
      prefilledToken: .mockSpdToken
    )
    
    let ex = expectation(description: "send-sol-transaction")
    store.sendToken(
      amount: "0.01"
    ) { success, errMsg in
      defer { ex.fulfill() }
      XCTAssertTrue(success)
    }
    
    waitForExpectations(timeout: 3) { error in
      XCTAssertNil(error)
    }
  }
  
  func testSendSolAmount() {
    let mockBalance = 47
    let sendSolAmountDecimalString = "0.01"
    
    let rpcService = BraveWallet.TestJsonRpcService()
    rpcService._chainId = { $1(BraveWallet.NetworkInfo.mockSolana.chainId) }
    rpcService._network = { $1(BraveWallet.NetworkInfo.mockSolana)}
    rpcService._solanaBalance = { _, _ , completion in
      completion(UInt64(mockBalance), .success, "")
    }
    rpcService._addObserver = { _ in }
    
    let walletService = BraveWallet.TestBraveWalletService()
    walletService._userAssets = { $2([.mockSolToken, .mockSpdToken]) }
    walletService._selectedCoin = { $0(BraveWallet.CoinType.sol) }
    
    let keyringService = BraveWallet.TestKeyringService()
    keyringService._selectedAccount = { $1("account-address") }
    keyringService._addObserver = { _ in }
    
    let solTxManagerProxy = BraveWallet.TestSolanaTxManagerProxy()
    solTxManagerProxy._makeSystemProgramTransferTxData = {_, _, lamports, completion in
      let solValueString = "10000000"
      XCTAssertNotNil(UInt64(solValueString))
      XCTAssertEqual(lamports, UInt64(solValueString)!)
      completion(.init(), .success, "")
    }
    
    let store = SendTokenStore(
      keyringService: keyringService,
      rpcService: rpcService,
      walletService: walletService,
      txService: MockTxService(),
      blockchainRegistry: MockBlockchainRegistry(),
      ethTxManagerProxy: MockEthTxManagerProxy(),
      solTxManagerProxy: solTxManagerProxy,
      prefilledToken: BraveWallet.NetworkInfo.mockSolana.nativeToken
    )
    
    let ex = expectation(description: "send-sol-transaction")
    store.sendToken(
      amount: sendSolAmountDecimalString
    ) { success, errMsg in
      defer { ex.fulfill() }
      XCTAssertTrue(success)
    }
    
    waitForExpectations(timeout: 3) { error in
      XCTAssertNil(error)
    }
  }
  
  func testSendSplTokenAmount() {
    let mockBalance = 47
    let sendSplAmountDecimalString = "0.01"
    
    let rpcService = BraveWallet.TestJsonRpcService()
    rpcService._chainId = { $1(BraveWallet.NetworkInfo.mockSolana.chainId) }
    rpcService._network = { $1(BraveWallet.NetworkInfo.mockSolana)}
    rpcService._solanaBalance = { _, _ , completion in
      completion(UInt64(mockBalance), .success, "")
    }
    rpcService._splTokenAccountBalance = { _, tokenAddress, _, completion in
      // balance of 0.1 SPD for token
      guard tokenAddress.caseInsensitiveCompare(BraveWallet.BlockchainToken.mockSpdToken.contractAddress) == .orderedSame else {
        completion("", UInt8(0), "", .internalError, "")
        XCTFail("Unexpected spl token balance fetched")
        return
      }
      completion("100000", UInt8(6), "0.1", .success, "")
    }
    rpcService._addObserver = { _ in }
    
    let walletService = BraveWallet.TestBraveWalletService()
    walletService._userAssets = { $2([.mockSolToken, .mockSpdToken]) }
    walletService._selectedCoin = { $0(BraveWallet.CoinType.sol) }
    
    let keyringService = BraveWallet.TestKeyringService()
    keyringService._selectedAccount = { $1("account-address") }
    keyringService._addObserver = { _ in }
    
    let solTxManagerProxy = BraveWallet.TestSolanaTxManagerProxy()
    solTxManagerProxy._makeTokenProgramTransferTxData = { _, _, _, amount, completion in
      let splValueString = "10000"
      XCTAssertNotNil(UInt64(splValueString))
      XCTAssertEqual(amount, UInt64(splValueString)!)
      completion(.init(), .success, "")
    }
    
    let store = SendTokenStore(
      keyringService: keyringService,
      rpcService: rpcService,
      walletService: walletService,
      txService: MockTxService(),
      blockchainRegistry: MockBlockchainRegistry(),
      ethTxManagerProxy: MockEthTxManagerProxy(),
      solTxManagerProxy: solTxManagerProxy,
      prefilledToken: .mockSpdToken
    )
    
    let ex = expectation(description: "send-spl-transaction")
    store.sendToken(
      amount: sendSplAmountDecimalString
    ) { success, errMsg in
      defer { ex.fulfill() }
      XCTAssertTrue(success)
    }
    
    waitForExpectations(timeout: 3) { error in
      XCTAssertNil(error)
    }
  }
}
