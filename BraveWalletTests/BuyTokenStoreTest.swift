// Copyright 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation

import XCTest
import Combine
import BraveCore
@testable import BraveWallet

class BuyTokenStoreTests: XCTestCase {
    func testPrefilledToken() {
        var store = BuyTokenStore(
            tokenRegistry: TestTokenRegistry(),
            rpcController: TestEthJsonRpcController(),
            prefilledToken: nil
        )
        XCTAssertNil(store.selectedBuyToken)
        
        store = BuyTokenStore(
            tokenRegistry: TestTokenRegistry(),
            rpcController: TestEthJsonRpcController(),
            prefilledToken: .eth
        )
        XCTAssertEqual(store.selectedBuyToken?.symbol.lowercased(), BraveWallet.ERCToken.eth.symbol.lowercased())
    }
    
}
