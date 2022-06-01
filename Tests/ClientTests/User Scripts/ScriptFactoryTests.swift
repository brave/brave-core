// Copyright 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import XCTest
@testable import Brave

class ScriptFactoryTests: XCTestCase {
  private let scriptFactory = ScriptFactory()

  func testLoadingFarblingProtectionScript() throws {
    // Given
    // Different ETLD+1s
    let etld = URL(string: "https://example.com")!.baseDomain!
    let etld2 = URL(string: "https://brave.com")!.baseDomain!

    // When
    // Making scripts from the different ETLD+1s
    let script1 = try scriptFactory.makeScript(for: .farblingProtection(etld: etld))
    let script2 = try scriptFactory.makeScript(for: .farblingProtection(etld: etld))
    let script3 = try scriptFactory.makeScript(for: .farblingProtection(etld: etld2))

    // Then
    // Test cached value is returned (i.e. Same script pointer)
    XCTAssertEqual(script1, script2)
    // Test the script is not modfied differently (i.e. same seed is injected)
    XCTAssertEqual(script1.source, script2.source)
    // Test no existing cache is not returned (i.e. Different script pointer)
    XCTAssertNotEqual(script1, script3)
    // Test the script is modified differently (i.e. different seed is injected)
    XCTAssertNotEqual(script1.source, script3.source)
  }

  func testLoadingScriptDomainScripts() throws {
    for domainUserScript in DomainUserScript.allCases {
      XCTAssertNoThrow(
        try scriptFactory.makeScript(for: .domainUserScript(domainUserScript))
      )
    }
  }
}
