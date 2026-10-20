// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveTransferCore
import XCTest

final class ProtoTests: XCTestCase {
  func testEncodeVectors() throws {
    let data = try Vectors.load("proto_messages.json")
    let cases = objects(data["encode"])
    XCTAssertFalse(cases.isEmpty)
    for c in cases {
      let type = c["type"] as! String
      let name = c["name"] as! String
      let encoded = c["encoded"] as! String
      XCTAssertEqual(encode(type: type, json: c["value"] as! JSONObject).hexString, encoded, name)
      XCTAssertEqual(try decodeAndReencode(type: type, hex(encoded)).hexString, encoded, name)
    }
  }

  func testDecodeVectors() throws {
    let data = try Vectors.load("proto_messages.json")
    for c in objects(data["decode"]) {
      XCTAssertEqual(
        try decodeAndReencode(type: c["type"] as! String, hex(c["encoded"])).hexString,
        c["canonical"] as? String, c["name"] as! String)
    }
  }

  func testInvalidVectors() throws {
    let data = try Vectors.load("proto_messages.json")
    for c in objects(data["invalid"]) {
      XCTAssertThrowsError(
        try decodeAndReencode(type: c["type"] as! String, hex(c["encoded"])), c["name"] as! String)
    }
  }

  func testUnknownEnumValuesSurviveDecoding() throws {
    let item = try OfferItem(serialized: [0x10, 0x63])
    XCTAssertEqual(item.kind.rawValue, 99)
    XCTAssertEqual(item.serialized(), [0x10, 0x63])
  }
}
