/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

import Foundation
import Shared
import XCTest

/**
 * Test for our own utils.
 */
class UtilsTests: XCTestCase {

  func testChunk() {
    let examples: [([Int], Int, [[Int]])] = [
      ([], 2, []),
      ([1, 2], 0, [[1], [2]]),
      ([1, 2], 1, [[1], [2]]),
      ([1, 2, 3], 2, [[1, 2], [3]]),
      ([1, 2], 3, [[1, 2]]),
      ([1, 2, 3], 1, [[1], [2], [3]]),
    ]
    for (arr, by, expected) in examples {
      // Turn the ArraySlices back into Arrays for comparison.
      let actual = chunk(arr as [Int], by: by).map { Array($0) }
      XCTAssertEqual(expected as NSArray, actual as NSArray)  //wtf. why is XCTAssert being so weeird
    }
  }
}
