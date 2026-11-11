// Copyright 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import Foundation
import XCTest

@testable import BraveWallet

class WalletStringExtensionTests: XCTestCase {
  func testHasUnknownUnicode() {
    let stringHasOnlyKnownUnicode = "hello this is message is in ascii chars only"
    XCTAssertFalse(stringHasOnlyKnownUnicode.hasUnknownUnicode)

    let stringsHasUnknownUnicode = "\u{202E} EVIL"
    XCTAssertTrue(stringsHasUnknownUnicode.hasUnknownUnicode)
  }

  func testHasUnexpectedCharacters() {
    XCTAssertFalse("hello this is message is in ascii chars only".hasUnexpectedCharacters)
    XCTAssertFalse("Hello\\0world".hasUnexpectedCharacters)

    XCTAssertTrue("Hello\0world".hasUnexpectedCharacters)
    XCTAssertTrue("Hello\nworld".hasUnexpectedCharacters)
    XCTAssertTrue("Sign into \u{202E} EVIL".hasUnexpectedCharacters)
    XCTAssertTrue("Hello\0\n\u{202E}world".hasUnexpectedCharacters)

    XCTAssertTrue("Hello\tworld".hasUnexpectedCharacters)
    XCTAssertTrue("Hello\rworld".hasUnexpectedCharacters)
    XCTAssertTrue("Hello\u{0B}world".hasUnexpectedCharacters)
    XCTAssertTrue("Hello\u{0C}world".hasUnexpectedCharacters)
    XCTAssertTrue("Hello\u{08}world".hasUnexpectedCharacters)
    XCTAssertTrue("Hello\u{1B}world".hasUnexpectedCharacters)
    XCTAssertTrue("Hello\u{7F}world".hasUnexpectedCharacters)
  }

  func testHasControlCharacters() {
    XCTAssertFalse("hello this is message is in ascii chars only".hasControlCharacters)

    // Space is printable, and null and newline have their own detection.
    XCTAssertFalse("Hello world".hasControlCharacters)
    XCTAssertFalse("Hello\0world".hasControlCharacters)
    XCTAssertFalse("Hello\nworld".hasControlCharacters)

    // Non-ASCII is not a control character.
    XCTAssertFalse("Sign into \u{202E} EVIL".hasControlCharacters)

    XCTAssertTrue("Hello\tworld".hasControlCharacters)
    XCTAssertTrue("Hello\rworld".hasControlCharacters)
    XCTAssertTrue("Hello\u{0B}world".hasControlCharacters)
    XCTAssertTrue("Hello\u{0C}world".hasControlCharacters)
    XCTAssertTrue("Hello\u{08}world".hasControlCharacters)
    XCTAssertTrue("Hello\u{1B}world".hasControlCharacters)
    XCTAssertTrue("Hello\u{7F}world".hasControlCharacters)
  }

  func testVisibleNullBytes() {
    XCTAssertEqual("Hello world".visibleNullBytes, "Hello world")

    // Text after a null must survive, since a text view would otherwise drop it.
    XCTAssertEqual("Hello\0world".visibleNullBytes, "Hello\u{2400}world")
    XCTAssertEqual("\0\0".visibleNullBytes, "\u{2400}\u{2400}")

    // A literal backslash-zero is ordinary text and stays untouched.
    XCTAssertEqual("Hello\\0world".visibleNullBytes, "Hello\\0world")
  }

  func testHasNullBytes() {
    XCTAssertFalse("hello this is message is in ascii chars only".hasNullBytes)

    // A literal backslash followed by a zero is not a null byte.
    XCTAssertFalse("Hello\\0world".hasNullBytes)

    XCTAssertTrue("Hello\0world".hasNullBytes)
    XCTAssertTrue("\0".hasNullBytes)
    XCTAssertTrue("Hello\0\u{202E} EVIL".hasNullBytes)
  }

  func testHasNewLines() {
    XCTAssertFalse("Here is one sentence.".hasNewLines)

    // A literal backslash followed by an n is not a newline.
    XCTAssertFalse("Here is one sentence.\\nAnd here is another.".hasNewLines)

    // A single newline is enough.
    XCTAssertTrue("Here is one sentence.\nAnd here is another.".hasNewLines)
    XCTAssertTrue("\n".hasNewLines)
    XCTAssertTrue("Main Message\n\n\n\n\nEvil payload is below".hasNewLines)

    // Other whitespace and control chars are not newlines.
    XCTAssertFalse("Hello\tworld".hasNewLines)
    XCTAssertFalse("Hello\rworld".hasNewLines)
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
    let string =
      "Main Message\nEvil payload is below \n¶\npaylod still loooks good\nNew Line\n¶\n\u{202E} EVIL"
    let printable =
      "Main Message\nEvil payload is below \n\\u{00B6}\npaylod still loooks good\nNew Line\n\\u{00B6}\n\\u{202E} EVIL"
    XCTAssertEqual(string.printableWithUnknownUnicode, printable)

    // Real newlines are kept as line breaks.
    XCTAssertEqual("Hello\nworld".printableWithUnknownUnicode, "Hello\nworld")

    // Null bytes render as an escape, so they are no longer invisible.
    XCTAssertEqual("Hello\0world".printableWithUnknownUnicode, "Hello\\0world")

    // A literal backslash is escaped, so it stays distinguishable from the
    // escape emitted for a real null byte above.
    XCTAssertEqual("Hello\\0world".printableWithUnknownUnicode, "Hello\\\\0world")

    // Other control characters are escaped too.
    XCTAssertEqual("Hello\tworld".printableWithUnknownUnicode, "Hello\\tworld")
    XCTAssertEqual("Hello\rworld".printableWithUnknownUnicode, "Hello\\rworld")
    XCTAssertEqual("Hello\u{1B}world".printableWithUnknownUnicode, "Hello\\u{1B}world")
    XCTAssertEqual("Hello\u{7F}world".printableWithUnknownUnicode, "Hello\\u{7F}world")
  }
}
