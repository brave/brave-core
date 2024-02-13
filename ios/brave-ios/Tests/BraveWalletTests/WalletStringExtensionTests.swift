// Copyright 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import XCTest
@testable import BraveWallet

import Foundation

class WalletStringExtensionTests: XCTestCase {
  func testHasUnknownUnicode() {
    let stringHasOnlyKnownUnicode = "hello this is message is in ascii chars only"
    XCTAssertFalse(stringHasOnlyKnownUnicode.hasUnknownUnicode)
    
   let stringsHasUnknownUnicode = "\u{202E} EVIL"
    XCTAssertTrue(stringsHasUnknownUnicode.hasUnknownUnicode)
  }
  
  func testHasConsecutiveNewlines() {
    let stringHasNoNewlines = "Here is one sentence."
    XCTAssertFalse(stringHasNoNewlines.hasConsecutiveNewLines)
    
    let stringsHasNewlineButNoConsecutiveNewlines = "Here is one sentence.\nAnd here is another."
    XCTAssertFalse(stringsHasNewlineButNoConsecutiveNewlines.hasConsecutiveNewLines)
    
    let stringsHasNewlineBut2ConsecutiveNewlines = "Here is one sentence.\n\nAnd here is another."
    XCTAssertFalse(stringsHasNewlineBut2ConsecutiveNewlines.hasConsecutiveNewLines)
    
    let stringsHasNewlineBut3ConsecutiveNewlines = "Here is one sentence.\n\n\nAnd here is another."
    XCTAssertTrue(stringsHasNewlineBut3ConsecutiveNewlines.hasConsecutiveNewLines)
    
    let stringHasConsecutiveNewlines = "Main Message\n\n\n\n\nEvil payload is below"
    XCTAssertTrue(stringHasConsecutiveNewlines.hasConsecutiveNewLines)
  }
  
  func testPrintableWithUnknownUnicode() {
    let string = "Main Message\nEvil payload is below \n¶\npaylod still loooks good\nNew Line\n¶\n\u{202E} EVIL"
    let printable = "Main Message\nEvil payload is below \n\\u{00B6}\npaylod still loooks good\nNew Line\n\\u{00B6}\n\\u{202E} EVIL"
    XCTAssertEqual(string.printableWithUnknownUnicode, printable)
  }
}
