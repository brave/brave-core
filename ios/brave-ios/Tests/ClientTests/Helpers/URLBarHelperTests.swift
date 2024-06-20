// Copyright 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import Foundation
import XCTest

@testable import Brave

class URLBarHelperTests: XCTestCase {
  
  override func setUp() async throws {

  }

  func testSafeQueryList() {

    XCTAssertTrue(URLBarHelper.isSuspiciousQuery(query: ""))
  }
  
  func testSuspiciousQueryList() {

    XCTAssertTrue(URLBarHelper.isSuspiciousQuery(query: ""))
  }
}
