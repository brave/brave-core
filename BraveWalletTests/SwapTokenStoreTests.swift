// Copyright 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import XCTest
import BraveCore
@testable import BraveWallet

class SendSwapStoreTests: XCTestCase {
    
    func testDefaultSellBuyTokensOnMainnetWithoutPrefilledToken() {
        let store = SwapTokenStore(
            keyringController: TestKeyringController(),
            tokenRegistry: TestTokenRegistry(),
            rpcController: TestEthJsonRpcController(),
            assetRatioController: TestAssetRatioController(),
            swapController: TestSwapController(),
            transactionController: TestEthTxController(),
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
        let batToken: BraveWallet.ERCToken = .init(contractAddress: "", name: "Brave BAT", logo: "", isErc20: true, isErc721: false, symbol: "BAT", decimals: 18, visible: false, tokenId: "")
        let store = SwapTokenStore(
            keyringController: TestKeyringController(),
            tokenRegistry: TestTokenRegistry(),
            rpcController: TestEthJsonRpcController(),
            assetRatioController: TestAssetRatioController(),
            swapController: TestSwapController(),
            transactionController: TestEthTxController(),
            prefilledToken: batToken
        )
        let ex = expectation(description: "default-sell-buy-token-on-main")
        XCTAssertNotNil(store.selectedFromToken)
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
    
    func testDefaultSellBuyTokensOnRopstenWithoutPrefilledToken() {
        let rpcController = TestEthJsonRpcController()
        let store = SwapTokenStore(
            keyringController: TestKeyringController(),
            tokenRegistry: TestTokenRegistry(),
            rpcController: rpcController,
            assetRatioController: TestAssetRatioController(),
            swapController: TestSwapController(),
            transactionController: TestEthTxController(),
            prefilledToken: nil
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
    
    func testDefaultSellBuyTokensOnRopstenWithPrefilledToken() {
        let daiToken: BraveWallet.ERCToken = .init(contractAddress: "", name: "DAI Stablecoin", logo: "", isErc20: true, isErc721: false, symbol: "DAI", decimals: 18, visible: false, tokenId: "")
        let rpcController = TestEthJsonRpcController()
        let store = SwapTokenStore(
            keyringController: TestKeyringController(),
            tokenRegistry: TestTokenRegistry(),
            rpcController: rpcController,
            assetRatioController: TestAssetRatioController(),
            swapController: TestSwapController(),
            transactionController: TestEthTxController(),
            prefilledToken: daiToken
        )
        let ex = expectation(description: "default-sell-buy-token-on-ropsten")
        XCTAssertNotNil(store.selectedFromToken)
        XCTAssertNil(store.selectedToToken)
        
        rpcController.setNetwork(BraveWallet.RopstenChainId) { success in
            XCTAssertTrue(success)
            let testAccountInfo: BraveWallet.AccountInfo = .init()
            store.prepare(with: testAccountInfo) {
                defer { ex.fulfill() }
                XCTAssertEqual(store.selectedFromToken?.symbol.lowercased(), daiToken.symbol.lowercased())
                XCTAssertNotNil(store.selectedToToken)
                XCTAssertNotEqual(store.selectedToToken!.symbol.lowercased(), daiToken.symbol.lowercased())
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
            transactionController: TestEthTxController(),
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
            keyringController: TestKeyringController(),
            tokenRegistry: TestTokenRegistry(),
            rpcController: TestEthJsonRpcController(),
            assetRatioController: TestAssetRatioController(),
            swapController: TestSwapController(),
            transactionController: TestEthTxController(),
            prefilledToken: nil
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
            transactionController: TestEthTxController(),
            prefilledToken: nil
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
            transactionController: TestEthTxController(),
            prefilledToken: nil
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
            transactionController: TestEthTxController(),
            prefilledToken: nil
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

