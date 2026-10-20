// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveTransferCore
import XCTest

final class ValidationTests: XCTestCase {
  func testFilenameSanitizerVectors() throws {
    let data = try Vectors.load("filename_sanitizer.json")
    let cases = objects(data["cases"])
    XCTAssertFalse(cases.isEmpty)
    for c in cases {
      let input = c["input"] as! String
      let output = FilenameSanitizer.sanitize(input)
      XCTAssertEqual(output, c["output"] as? String, "input: \(input.debugDescription)")
      XCTAssertTrue((1...255).contains(output.utf8.count))
      XCTAssertEqual(FilenameSanitizer.sanitize(output), output)
    }
  }

  func testOfferValidationVectors() throws {
    let data = try Vectors.load("offer_validation.json")
    let cases = objects(data["cases"])
    XCTAssertFalse(cases.isEmpty)
    for c in cases {
      var offer = Offer(json: c["offer"] as! JSONObject)
      if let template = c["item_template"] as? JSONObject {
        offer.items = (1...uint32(c["item_count"])).map { id in
          var item = OfferItem(json: template)
          item.itemID = id
          return item
        }
      }
      let name = c["name"] as! String
      if c["valid"] as? Bool == true {
        XCTAssertNoThrow(try validateOffer(offer), name)
      } else {
        XCTAssertThrowsError(try validateOffer(offer), name)
      }
    }
  }
}
