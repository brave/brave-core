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
            keyringService: MockKeyringService(),
            blockchainRegistry: MockBlockchainRegistry(),
            rpcService: MockJsonRpcService(),
            assetRatioService: MockAssetRatioService(),
            swapService: MockSwapService(),
            txService: MockEthTxService(),
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
        let batToken: BraveWallet.BlockchainToken = .init(contractAddress: "", name: "Brave BAT", logo: "", isErc20: true, isErc721: false, symbol: "BAT", decimals: 18, visible: false, tokenId: "", coingeckoId: "")
        let store = SwapTokenStore(
            keyringService: MockKeyringService(),
            blockchainRegistry: MockBlockchainRegistry(),
            rpcService: MockJsonRpcService(),
            assetRatioService: MockAssetRatioService(),
            swapService: MockSwapService(),
            txService: MockEthTxService(),
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
        let rpcService = MockJsonRpcService()
        let store = SwapTokenStore(
            keyringService: MockKeyringService(),
            blockchainRegistry: MockBlockchainRegistry(),
            rpcService: rpcService,
            assetRatioService: MockAssetRatioService(),
            swapService: MockSwapService(),
            txService: MockEthTxService(),
            prefilledToken: nil
        )
        let ex = expectation(description: "default-sell-buy-token-on-ropsten")
        XCTAssertNil(store.selectedFromToken)
        XCTAssertNil(store.selectedToToken)
        
        rpcService.setNetwork(BraveWallet.RopstenChainId) { success in
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
        let daiToken: BraveWallet.BlockchainToken = .init(contractAddress: "", name: "DAI Stablecoin", logo: "", isErc20: true, isErc721: false, symbol: "DAI", decimals: 18, visible: false, tokenId: "", coingeckoId: "")
        let rpcService = MockJsonRpcService()
        let store = SwapTokenStore(
            keyringService: MockKeyringService(),
            blockchainRegistry: MockBlockchainRegistry(),
            rpcService: rpcService,
            assetRatioService: MockAssetRatioService(),
            swapService: MockSwapService(),
            txService: MockEthTxService(),
            prefilledToken: daiToken
        )
        let ex = expectation(description: "default-sell-buy-token-on-ropsten")
        XCTAssertNotNil(store.selectedFromToken)
        XCTAssertNil(store.selectedToToken)
        
        rpcService.setNetwork(BraveWallet.RopstenChainId) { success in
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
            keyringService: MockKeyringService(),
            blockchainRegistry: MockBlockchainRegistry(),
            rpcService: MockJsonRpcService(),
            assetRatioService: MockAssetRatioService(),
            swapService: MockSwapService(),
            txService: MockEthTxService(),
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
            assetRatioService: MockAssetRatioService(),
            swapService: MockSwapService(),
            txService: MockEthTxService(),
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
        let rpcService = MockJsonRpcService()
        let store = SwapTokenStore(
            keyringService: MockKeyringService(),
            blockchainRegistry: MockBlockchainRegistry(),
            rpcService: rpcService,
            assetRatioService: MockAssetRatioService(),
            swapService: MockSwapService(),
            txService: MockEthTxService(),
            prefilledToken: nil
        )
        let ex = expectation(description: "make-erc20-swap-transaction")
        store.setUpTest()
        store.state = .lowAllowance("test-spender-address")
        
        rpcService.setNetwork(BraveWallet.RopstenChainId) { success in
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
            keyringService: MockKeyringService(),
            blockchainRegistry: MockBlockchainRegistry(),
            rpcService: MockJsonRpcService(),
            assetRatioService: MockAssetRatioService(),
            swapService: MockSwapService(),
            txService: MockEthTxService(),
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
        let rpcService = MockJsonRpcService()
        let store = SwapTokenStore(
            keyringService: MockKeyringService(),
            blockchainRegistry: MockBlockchainRegistry(),
            rpcService: rpcService,
            assetRatioService: MockAssetRatioService(),
            swapService: MockSwapService(),
            txService: MockEthTxService(),
            prefilledToken: nil
        )
        let ex = expectation(description: "make-eth-swap-eip1559-transaction")
        store.setUpTest()
        store.state = .swap
        
        rpcService.setNetwork(BraveWallet.RopstenChainId) { success in
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

