// Copyright 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import XCTest
import Combine
import BraveCore
@testable import BraveWallet

class SendTokenStoreTests: XCTestCase {
    private var cancellables: Set<AnyCancellable> = []
    private let batSymbol = "BAT"
    
    func testPrefilledToken() {
        var store = SendTokenStore(
            keyringController: TestKeyringController(),
            rpcController: TestEthJsonRpcController(),
            walletService: TestBraveWalletService(),
            transactionController: TestEthTxController(),
            tokenRegistery: TestTokenRegistry(),
            prefilledToken: nil
        )
        XCTAssertNil(store.selectedSendToken)
        
        store = SendTokenStore(
            keyringController: TestKeyringController(),
            rpcController: TestEthJsonRpcController(),
            walletService: TestBraveWalletService(),
            transactionController: TestEthTxController(),
            tokenRegistery: TestTokenRegistry(),
            prefilledToken: .eth
        )
        XCTAssertEqual(store.selectedSendToken?.symbol.lowercased(), BraveWallet.ERCToken.eth.symbol.lowercased())
    }
    
    func testFetchAssets() {
        let store = SendTokenStore(
            keyringController: TestKeyringController(),
            rpcController: TestEthJsonRpcController(),
            walletService: TestBraveWalletService(),
            transactionController: TestEthTxController(),
            tokenRegistery: TestTokenRegistry(),
            prefilledToken: nil
        )
        let ex = expectation(description: "fetch-assets")
        XCTAssertNil(store.selectedSendToken) // Initial state
        store.$selectedSendToken.dropFirst().sink { token in
            defer { ex.fulfill() }
            guard let token = token else {
                XCTFail("Token was nil")
                return
            }
            XCTAssert(token.isETH) // Should end up showing ETH as the default asset
        }.store(in: &cancellables)
        store.fetchAssets()
        waitForExpectations(timeout: 3) { error in
            XCTAssertNil(error)
        }
    }
    
    func testMakeSendETHEIP1559Transaction() {
        let store = SendTokenStore(
            keyringController: TestKeyringController(),
            rpcController: TestEthJsonRpcController(),
            walletService: TestBraveWalletService(),
            transactionController: TestEthTxController(),
            tokenRegistery: TestTokenRegistry(),
            prefilledToken: .eth
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
        let rpcController = TestEthJsonRpcController()
        let store = SendTokenStore(
            keyringController: TestKeyringController(),
            rpcController: rpcController,
            walletService: TestBraveWalletService(),
            transactionController: TestEthTxController(),
            tokenRegistery: TestTokenRegistry(),
            prefilledToken: .eth
        )
        store.setUpTest()
        
        let ex = expectation(description: "send-eth-transaction")
        rpcController.setNetwork(BraveWallet.RopstenChainId) { success in
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
            keyringController: TestKeyringController(),
            rpcController: TestEthJsonRpcController(),
            walletService: TestBraveWalletService(),
            transactionController: TestEthTxController(),
            tokenRegistery: TestTokenRegistry(),
            prefilledToken: nil
        )
        let token: BraveWallet.ERCToken = .init(contractAddress: "0x0d8775f648430679a709e98d2b0cb6250d2887ef", name: "Basic Attention Token", logo: "", isErc20: true, isErc721: false, symbol: batSymbol, decimals: 18, visible: true, tokenId: "")
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
        let rpcController = TestEthJsonRpcController()
        let store = SendTokenStore(
            keyringController: TestKeyringController(),
            rpcController: rpcController,
            walletService: TestBraveWalletService(),
            transactionController: TestEthTxController(),
            tokenRegistery: TestTokenRegistry(),
            prefilledToken: .eth
        )
        store.setUpTest()
        
        let ex = expectation(description: "send-bat-transaction")
        rpcController.setNetwork(BraveWallet.RopstenChainId) { success in
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
}
