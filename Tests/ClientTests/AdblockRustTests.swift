// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import XCTest
import BraveCore
@testable import Brave

class AdblockRustTests: XCTestCase {
  // Taken from adblock-rust-ffi TestBasics()
  func testBasicBlocking() throws {
    let rules =
      """
          -advertisement-icon.
          -advertisement-management
          -advertisement.
          -advertisement/script.
          @@good-advertisement
      """

    AdblockEngine.setDomainResolver()
    let engine = try AdblockEngine(rules: rules)

    XCTAssertTrue(engine.shouldBlock(
      requestURL: URL(string: "http://example.com/-advertisement-icon.")!,
      sourceURL: URL(string: "https://example.com")!,
      resourceType: .xmlhttprequest,
      isAggressive: true
    ))
    
    XCTAssertFalse(engine.shouldBlock(
      requestURL: URL(string: "https://brianbondy.com")!,
      sourceURL: URL(string: "https://example.com")!,
      resourceType: .xmlhttprequest,
      isAggressive: true
    ))
    
    XCTAssertFalse(engine.shouldBlock(
      requestURL: URL(string: "http://example.com/good-advertisement-icon.")!,
      sourceURL: URL(string: "https://example.com")!,
      resourceType: .xmlhttprequest,
      isAggressive: true
    ))
  }
}
