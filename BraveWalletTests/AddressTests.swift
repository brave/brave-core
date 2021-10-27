// Copyright 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import XCTest
@testable import BraveWallet

class AddressTests: XCTestCase {
    func testAddressTruncation() {
        let address = "0xabcdef0123456789"
        XCTAssertEqual(address.truncatedAddress, "0xabcd…6789")
        
        let prefixlessAddress = "abcdef0123456789"
        XCTAssertEqual(prefixlessAddress.truncatedAddress, "abcd…6789")
    }
    
    func testRemovingHexPrefix() {
        let address = "0xabcdef0123456789"
        XCTAssertEqual(address.removingHexPrefix, "abcdef0123456789")
        
        let prefixlessAddress = "abcdef0123456789"
        XCTAssertEqual(prefixlessAddress.removingHexPrefix, "abcdef0123456789")
    }
}
