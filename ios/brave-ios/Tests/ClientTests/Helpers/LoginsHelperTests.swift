// Copyright 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveCore
import Foundation
import WebKit
import XCTest

@testable import Brave

class LoginsHelperTests: XCTestCase {

  func testFrameOriginModified() {
    let testFrameInfo1 = (scheme: "https", host: "www.test.com", port: 0)
    let testFrameInfo2 = (scheme: "https", host: "www.test.com", port: 8080)
    let testFrameInfo3 = (scheme: "https", host: "www.test.com", port: 8080)
    let testFrameInfo4 = (scheme: "http", host: "www.test.com", port: 8080)
    let testFrameInfo5 = (scheme: "https", host: "www.test1.com", port: 0)

    guard let testTabURL1 = URL(string: "https://www.test.com") else {
      XCTFail()
      return
    }

    guard let testTabURL2 = URL(string: "https://www.test.com:8080") else {
      XCTFail()
      return
    }

    guard let testTabURL3 = URL(string: "https://www.test.com") else {
      XCTFail()
      return
    }

    guard let testTabURL4 = URL(string: "https://www.test.com:2020") else {
      XCTFail()
      return
    }

    guard let testTabURL5 = URL(string: "https://www.test2.com") else {
      XCTFail()
      return
    }

    let modifiedFrameCheck1 = LoginsScriptHandler.checkIsSameFrame(
      url: testTabURL1,
      frameScheme: testFrameInfo1.scheme,
      frameHost: testFrameInfo1.host,
      framePort: testFrameInfo1.port
    )

    let modifiedFrameCheck2 = LoginsScriptHandler.checkIsSameFrame(
      url: testTabURL2,
      frameScheme: testFrameInfo2.scheme,
      frameHost: testFrameInfo2.host,
      framePort: testFrameInfo2.port
    )

    let modifiedFrameCheck3 = LoginsScriptHandler.checkIsSameFrame(
      url: testTabURL3,
      frameScheme: testFrameInfo3.scheme,
      frameHost: testFrameInfo3.host,
      framePort: testFrameInfo3.port
    )

    let modifiedFrameCheck4 = LoginsScriptHandler.checkIsSameFrame(
      url: testTabURL4,
      frameScheme: testFrameInfo4.scheme,
      frameHost: testFrameInfo4.host,
      framePort: testFrameInfo4.port
    )

    let modifiedFrameCheck5 = LoginsScriptHandler.checkIsSameFrame(
      url: testTabURL5,
      frameScheme: testFrameInfo5.scheme,
      frameHost: testFrameInfo5.host,
      framePort: testFrameInfo5.port
    )

    XCTAssertTrue(modifiedFrameCheck1, "Same host, same scheme, no port")
    XCTAssertTrue(modifiedFrameCheck2, "Same host, same scheme, same port")
    XCTAssertFalse(modifiedFrameCheck3, "Same host, same scheme, additional port")
    XCTAssertFalse(modifiedFrameCheck4, "Same host, different scheme, different port")
    XCTAssertFalse(modifiedFrameCheck5, "Different host, same scheme, same port")
  }
}
