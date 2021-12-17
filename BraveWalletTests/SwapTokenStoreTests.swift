// Copyright 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import XCTest
import BraveCore
@testable import BraveWallet

class SendSwapStoreTests: XCTestCase {
    
    func testDefaultSellBuyTokensOnMainnet() {
        let store = SwapTokenStore(
            keyringController: TestKeyringController(),
            tokenRegistry: TestTokenRegistry(),
            rpcController: TestEthJsonRpcController(),
            assetRatioController: TestAssetRatioController(),
            swapController: TestSwapController(),
            transactionController: TestEthTxController()
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
    
    func testDefaultSellBuyTokensOnRopsten() {
        let rpcController = TestEthJsonRpcController()
        let store = SwapTokenStore(
            keyringController: TestKeyringController(),
            tokenRegistry: TestTokenRegistry(),
            rpcController: rpcController,
            assetRatioController: TestAssetRatioController(),
            swapController: TestSwapController(),
            transactionController: TestEthTxController()
        )
        let ex = expectation(description: "default-sell-buy-token-on-ropsten")
        XCTAssertNil(store.selectedFromToken)
        XCTAssertNil(store.selectedToToken)
        
        rpcController.setNetwork(BraveWallet.RopstenChainId) { success in
            XCTAssertTrue(success)
            let testAccountInfo: BraveWallet.AccountInfo = .init()
            store.prepare(with: testAccountInfo) {
                defer { ex.fulfill() }
                XCTAssertEqual(store.selectedFromToken?.symbol.lowercased(), "eth")
                XCTAssertEqual(store.selectedToToken?.symbol.lowercased(), "dai")
            }
        }
        waitForExpectations(timeout: 3) { error in
            XCTAssertNil(error)
        }
    }
    
    func testFetchPriceQuote() {
        let store = SwapTokenStore(
            keyringController: TestKeyringController(),
            tokenRegistry: TestTokenRegistry(),
            rpcController: TestEthJsonRpcController(),
            assetRatioController: TestAssetRatioController(),
            swapController: TestSwapController(),
            transactionController: TestEthTxController()
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
            keyringController: TestKeyringController(),
            tokenRegistry: TestTokenRegistry(),
            rpcController: TestEthJsonRpcController(),
            assetRatioController: TestAssetRatioController(),
            swapController: TestSwapController(),
            transactionController: TestEthTxController()
        )
        let ex = expectation(description: "make-erc20-eip1559-swap-transaction")
        store.setUpTest()
        store.state = .lowAllowance("test-spender-address")
        store.prepareSwap()
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
        let rpcController = TestEthJsonRpcController()
        let store = SwapTokenStore(
            keyringController: TestKeyringController(),
            tokenRegistry: TestTokenRegistry(),
            rpcController: rpcController,
            assetRatioController: TestAssetRatioController(),
            swapController: TestSwapController(),
            transactionController: TestEthTxController()
        )
        let ex = expectation(description: "make-erc20-swap-transaction")
        store.setUpTest()
        store.state = .lowAllowance("test-spender-address")
        
        rpcController.setNetwork(BraveWallet.RopstenChainId) { success in
            XCTAssertTrue(success)
            store.prepareSwap()
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
            keyringController: TestKeyringController(),
            tokenRegistry: TestTokenRegistry(),
            rpcController: TestEthJsonRpcController(),
            assetRatioController: TestAssetRatioController(),
            swapController: TestSwapController(),
            transactionController: TestEthTxController()
        )
        let ex = expectation(description: "make-eth-swap-eip1559-transaction")
        store.setUpTest()
        store.state = .swap
        store.prepareSwap()
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
        let rpcController = TestEthJsonRpcController()
        let store = SwapTokenStore(
            keyringController: TestKeyringController(),
            tokenRegistry: TestTokenRegistry(),
            rpcController: rpcController,
            assetRatioController: TestAssetRatioController(),
            swapController: TestSwapController(),
            transactionController: TestEthTxController()
        )
        let ex = expectation(description: "make-eth-swap-eip1559-transaction")
        store.setUpTest()
        store.state = .swap
        
        rpcController.setNetwork(BraveWallet.RopstenChainId) { success in
            XCTAssertTrue(success)
            store.prepareSwap()
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
}

