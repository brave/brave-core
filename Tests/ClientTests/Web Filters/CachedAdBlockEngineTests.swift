// Copyright 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import XCTest
import BraveCore
import Data
@testable import Brave

final class CachedAdBlockEngineTests: XCTestCase {
  func testEngineMemoryManagment() throws {
    var engine: AdblockEngine? = AdblockEngine()
    weak var weakEngine: AdblockEngine? = engine
    
    var cachedEngine: CachedAdBlockEngine? = CachedAdBlockEngine(
      engine: engine!, source: .adBlock, serialQueue: DispatchQueue(label: "test")
    )
    
    XCTAssertNotNil(cachedEngine)
    XCTAssertNotNil(weakEngine)
    
    engine = nil
    cachedEngine = nil
    
    XCTAssertNil(engine)
    XCTAssertNil(weakEngine)
    XCTAssertNil(cachedEngine)
  }
}
