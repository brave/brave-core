// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import Foundation
import Shared
import XCTest

class ArrayExtensionTests: XCTestCase {
  override func setUp() {
    super.setUp()
  }

  override func tearDown() {
    super.tearDown()
  }

  func testSplitEvery() {
    // Edge cases
    let emptyArray = [Int]()
    XCTAssertEqual(emptyArray.splitEvery(3), [])
    XCTAssertEqual([1, 2, 3].splitEvery(-1), [])
    XCTAssertEqual([1, 2, 3].splitEvery(100), [[1, 2, 3]])
    XCTAssertEqual([1].splitEvery(1), [[1]])
    XCTAssertEqual([1].splitEvery(0), [])

    XCTAssertEqual([1, 2, 3].splitEvery(1), [[1], [2], [3]])
    XCTAssertEqual([1, 2, 3, 4, 5].splitEvery(2), [[1, 2], [3, 4], [5]])
  }
}
