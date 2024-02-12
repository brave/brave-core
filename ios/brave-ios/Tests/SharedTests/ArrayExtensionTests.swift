/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

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

  func testUnique() {
    let a = [1, 2, 3, 4, 5, 6, 1, 2]
    let result = a.unique { return $0 }
    XCTAssertEqual(result, [1, 2, 3, 4, 5, 6])

    let b = [1, 2, 3]
    let resultB = b.unique { return $0 }
    XCTAssertEqual(resultB, [1, 2, 3])
  }

  func testUnion() {
    let a = [1, 2, 3, 4, 5, 6]
    let b = [7, 8, 9, 10]
    XCTAssertEqual(
      a.union(b) { return $0 },
      [1, 2, 3, 4, 5, 6, 7, 8, 9, 10])

    let c = [1, 2, 3, 4, 5, 6]
    let d = [4, 5, 6, 7, 8, 9, 10]
    XCTAssertEqual(c.union(d) { return $0 }, [1, 2, 3, 4, 5, 6, 7, 8, 9, 10])

    let e = [1, 2, 3, 4, 5, 6, 7, 8, 9, 10]
    let f = [4, 5, 6, 7, 8, 9, 10]
    XCTAssertEqual(e.union(f) { return $0 }, [1, 2, 3, 4, 5, 6, 7, 8, 9, 10])

    let g = [1, 2, 3, 4, 5, 6, 7, 8, 9, 10]
    let h = [Int]()
    XCTAssertEqual(g.union(h) { return $0 }, [1, 2, 3, 4, 5, 6, 7, 8, 9, 10])

    let i = [Int]()
    let j = [1, 2, 3, 4, 5, 6, 7, 8, 9, 10]
    XCTAssertEqual(i.union(j) { return $0 }, [1, 2, 3, 4, 5, 6, 7, 8, 9, 10])
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
  
  func testContiguiousUntil() {
    let testArray = [2, 4, 6, 8, 10]
    let emptyArray = [Int]()
    
    XCTAssertEqual(
      testArray.contiguousUntil(condition: { $0 % 2 == 0 }),
      testArray[0...]
    )
    
    XCTAssertEqual(
      testArray.contiguousUntil(condition: { $0 % 2 != 0 }),
      emptyArray[0...]
    )
    
    XCTAssertEqual(
      testArray.contiguousUntil(condition: { $0 <= 6 }),
      testArray[0...2]
    )
    
    XCTAssertEqual(
      emptyArray.contiguousUntil(condition: { $0 > 0 }),
      emptyArray[0...]
    )
  }
}
