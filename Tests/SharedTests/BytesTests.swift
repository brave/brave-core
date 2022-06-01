// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import XCTest
import Shared

class BytesTests: XCTestCase {

  func testGenerateRandomBytes() {

    let rollCount = 200
    let bytesLength: UInt = 256

    var resultsSet = Set<Data>()

    for _ in 0..<rollCount {
      resultsSet.insert(Bytes.generateRandomBytes(bytesLength))
    }

    // Sets not equal means we found a duplicate, which should not happen for such small sample size.
    XCTAssertEqual(resultsSet.count, rollCount)
  }
}
