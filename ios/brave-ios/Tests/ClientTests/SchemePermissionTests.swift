// Copyright 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation
import XCTest
import Storage
import BraveCore

@testable import Brave

// MARK: SchemePermissionTests

class SchemePermissionTests: XCTestCase {

  enum SchemeTestType: String {
    case http
    case https
    case about
    case javascript
    case whatsapp

    var description: String {
      switch self {
      case .http:
        return "Http URL Scheme in new tab"
      case .https:
        return "Https URL Scheme in new tab"
      case .about:
        return "About URL Scheme in new tab"
      case .javascript:
        return "Javascript URL Scheme in new tab"
      case .whatsapp:
        return "WhatsApp URL Scheme in new tab"
      }
    }

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
      case .whatsapp:
        return "whatsapp://send?text=Test"
      }
    }
  }
  
  // MARK: Internal

  func testShouldRequestOpenPopup() {
    let urlRequestHttp = urlRequestForScheme(.http)
    let urlRequestHttps = urlRequestForScheme(.https)
    let urlRequestJavascript = urlRequestForScheme(.javascript)
    let urlRequestAbout = urlRequestForScheme(.about)
    let urlRequestWhatsApp = urlRequestForScheme(.whatsapp)

    // Test Http URL Scheme
    XCTAssertTrue(urlRequestHttp.request.url!.shouldRequestBeOpenedAsPopup(), urlRequestHttp.comment)

    // Test Https URL Scheme
    XCTAssertTrue(urlRequestHttps.request.url!.shouldRequestBeOpenedAsPopup(), urlRequestHttps.comment)

    // Test Javascript URL Scheme
    XCTAssertTrue(urlRequestJavascript.request.url!.shouldRequestBeOpenedAsPopup(), urlRequestJavascript.comment)

    // Test About URL Scheme
    XCTAssertTrue(urlRequestAbout.request.url!.shouldRequestBeOpenedAsPopup(), urlRequestAbout.comment)

    // Test Whatapp URL Scheme
    XCTAssertTrue(urlRequestWhatsApp.request.url!.shouldRequestBeOpenedAsPopup(), urlRequestWhatsApp.comment)
  }

  private func urlRequestForScheme(_ type: SchemeTestType) -> (request: URLRequest, comment: String) {
    (request: URLRequest(url: URL(string: type.url)!), comment: type.description)
  }

}
