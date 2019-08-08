// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import XCTest
@testable import Client

class AdblockRustTests: XCTestCase {

    // Taken from adblock-rust-ffi TestBasics()
    func testBasicBlocking() {
        let rules =
        """
            -advertisement-icon.
            -advertisement-management
            -advertisement.
            -advertisement/script.
            @@good-advertisement
        """
        
        let engine = AdblockRustEngine(rules: rules)
        
        XCTAssert(engine.shouldBlock(requestUrl: "http://example.com/-advertisement-icon.", requestHost: "example.com", sourceHost: "example.com"))
        
        XCTAssertFalse(engine.shouldBlock(requestUrl: "https://brianbondy.com", requestHost: "https://brianbondy.com", sourceHost: "example.com"))
        
        XCTAssertFalse(engine.shouldBlock(requestUrl: "http://example.com/good-advertisement-icon.", requestHost: "example.com", sourceHost: "example.com"))
    }

}
