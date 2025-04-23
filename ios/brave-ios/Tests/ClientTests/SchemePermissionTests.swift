// Copyright 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveCore
import Foundation
import Storage
import XCTest

@testable import Brave

// MARK: SchemePermissionTests

class SchemePermissionTests: XCTestCase {

  enum SchemeTestType: String {
    case http
    case https
    case about
    case javascript
    case blob
    case data

    var url: String {
      switch self {
      case .http:
        return "http://test.com/"
      case .https:
        return "https://test.com/"
      case .about:
        return "about:Test"
      case .javascript:
        return "javascript:alert(Test)"
      case .blob:
        return "blob:https://test.com/1234"
      case .data:
        return "data:,Test"
      }
    }
  }

  // MARK: Internal

  func testShouldRequestOpenPopup() {
    let urlRequestHttp = urlRequestForScheme(.http)
    let urlRequestHttps = urlRequestForScheme(.https)
    let urlRequestJavascript = urlRequestForScheme(.javascript)
    let urlRequestAbout = urlRequestForScheme(.about)
    let urlRequestBlob = urlRequestForScheme(.blob)
    let urlRequestData = urlRequestForScheme(.data)

    // Test Http URL Scheme
    XCTAssertTrue(urlRequestHttp.url!.shouldRequestBeOpenedAsPopup())

    // Test Https URL Scheme
    XCTAssertTrue(urlRequestHttps.url!.shouldRequestBeOpenedAsPopup())

    // Test Javascript URL Scheme
    XCTAssertTrue(urlRequestJavascript.url!.shouldRequestBeOpenedAsPopup())

    // Test About URL Scheme
    XCTAssertTrue(urlRequestAbout.url!.shouldRequestBeOpenedAsPopup())

    // Test blob URL Scheme
    XCTAssertTrue(urlRequestBlob.url!.shouldRequestBeOpenedAsPopup())

    // Test data URL Scheme
    XCTAssertTrue(urlRequestData.url!.shouldRequestBeOpenedAsPopup())
  }

  private func urlRequestForScheme(_ type: SchemeTestType) -> URLRequest {
    return URLRequest(url: URL(string: type.url)!)
  }
}
