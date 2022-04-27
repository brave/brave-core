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
    var store = SendTokenStore(
      keyringService: MockKeyringService(),
      rpcService: MockJsonRpcService(),
      walletService: MockBraveWalletService(),
      txService: MockTxService(),
      blockchainRegistry: MockBlockchainRegistry(),
      ethTxManagerProxy: MockEthTxManagerProxy(),
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
      prefilledToken: .previewToken
    )
    XCTAssertEqual(store.selectedSendToken?.symbol.lowercased(), BraveWallet.BlockchainToken.previewToken.symbol.lowercased())
  }

  func testFetchAssets() {
    let rpcService = MockJsonRpcService()
    let store = SendTokenStore(
      keyringService: MockKeyringService(),
      rpcService: rpcService,
      walletService: MockBraveWalletService(),
      txService: MockTxService(),
      blockchainRegistry: MockBlockchainRegistry(),
      ethTxManagerProxy: MockEthTxManagerProxy(),
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
      rpcService.network { network in
        XCTAssertEqual(token.symbol, network.symbol)
      }
    }.store(in: &cancellables)
    store.fetchAssets()
    waitForExpectations(timeout: 3) { error in
      XCTAssertNil(error)
    }
  }

  func testMakeSendETHEIP1559Transaction() {
    let store = SendTokenStore(
      keyringService: MockKeyringService(),
      rpcService: MockJsonRpcService(),
      walletService: MockBraveWalletService(),
      txService: MockTxService(),
      blockchainRegistry: MockBlockchainRegistry(),
      ethTxManagerProxy: MockEthTxManagerProxy(),
      prefilledToken: .previewToken
    )
    store.setUpTest()
    let ex = expectation(description: "send-eth-eip1559-transaction")
    store.sendToken(amount: "0.01") { success in
      defer { ex.fulfill() }
      XCTAssertTrue(success)
    }
    waitForExpectations(timeout: 3) { error in
      XCTAssertNil(error)
    }
  }

  func testMakeSendETHTransaction() {
    let rpcService = MockJsonRpcService()
    let store = SendTokenStore(
      keyringService: MockKeyringService(),
      rpcService: rpcService,
      walletService: MockBraveWalletService(),
      txService: MockTxService(),
      blockchainRegistry: MockBlockchainRegistry(),
      ethTxManagerProxy: MockEthTxManagerProxy(),
      prefilledToken: .previewToken
    )
    store.setUpTest()

    let ex = expectation(description: "send-eth-transaction")
    rpcService.setNetwork(BraveWallet.RopstenChainId) { success in
      XCTAssertTrue(success)
      store.sendToken(amount: "0.01") { success in
        defer { ex.fulfill() }
        XCTAssertTrue(success)
      }
    }
    waitForExpectations(timeout: 3) { error in
      XCTAssertNil(error)
    }
  }

  func testMakeSendERC20EIP1559Transaction() {
    let store = SendTokenStore(
      keyringService: MockKeyringService(),
      rpcService: MockJsonRpcService(),
      walletService: MockBraveWalletService(),
      txService: MockTxService(),
      blockchainRegistry: MockBlockchainRegistry(),
      ethTxManagerProxy: MockEthTxManagerProxy(),
      prefilledToken: nil
    )
    let token: BraveWallet.BlockchainToken = .init(contractAddress: "0x0d8775f648430679a709e98d2b0cb6250d2887ef", name: "Basic Attention Token", logo: "", isErc20: true, isErc721: false, symbol: batSymbol, decimals: 18, visible: true, tokenId: "", coingeckoId: "", chainId: "", coin: .eth)
    store.selectedSendToken = token
    store.setUpTest()

    let ex = expectation(description: "send-bat-eip1559-transaction")
    store.sendToken(amount: "0.01") { success in
      defer { ex.fulfill() }
      XCTAssertTrue(success)
    }
    waitForExpectations(timeout: 3) { error in
      XCTAssertNil(error)
    }
  }

  func testMakeSendERC20Transaction() {
    let rpcService = MockJsonRpcService()
    let store = SendTokenStore(
      keyringService: MockKeyringService(),
      rpcService: rpcService,
      walletService: MockBraveWalletService(),
      txService: MockTxService(),
      blockchainRegistry: MockBlockchainRegistry(),
      ethTxManagerProxy: MockEthTxManagerProxy(),
      prefilledToken: .previewToken
    )
    store.setUpTest()

    let ex = expectation(description: "send-bat-transaction")
    rpcService.setNetwork(BraveWallet.RopstenChainId) { success in
      XCTAssertTrue(success)
      store.sendToken(amount: "0.01") { success in
        defer { ex.fulfill() }
        XCTAssertTrue(success)
      }
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
    
    let keyringService = BraveWallet.TestKeyringService()
    keyringService._selectedAccount = { $1("account-address") }
    keyringService._addObserver = { _ in }
    
    let store = SendTokenStore(
      keyringService: keyringService,
      rpcService: rpcService,
      walletService: walletService,
      txService: MockTxService(),
      blockchainRegistry: MockBlockchainRegistry(),
      ethTxManagerProxy: MockEthTxManagerProxy(),
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
}
