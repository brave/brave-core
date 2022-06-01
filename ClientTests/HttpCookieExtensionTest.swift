// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import XCTest
@testable import Brave
import WebKit

class HttpCookieExtensionTest: XCTestCase {

  func testSaveAndLoadCookie() {
    // Set some cookies here
    let expectation = XCTestExpectation(description: "Cookie is loaded from file")

    let properties: [HTTPCookiePropertyKey: Any] = [
      HTTPCookiePropertyKey.name: "BraveCookieTest",
      HTTPCookiePropertyKey.domain: "brave.com",
      .path: "/",
      .value: "something",
      .secure: "TRUE",
      .expires: NSDate(timeIntervalSinceNow: 30),
    ]
    if let cookie = HTTPCookie(properties: properties) {
      WKWebsiteDataStore.default().fetchDataRecords(ofTypes: [WKWebsiteDataTypeCookies]) { _ in }
      WKWebsiteDataStore.default().httpCookieStore.setCookie(cookie) {
        HTTPCookie.saveToDisk(completion: { _ in
          WKWebsiteDataStore.default().fetchDataRecords(ofTypes: [WKWebsiteDataTypeCookies]) { _ in }
          WKWebsiteDataStore.default().httpCookieStore.delete(
            cookie,
            completionHandler: {
              HTTPCookie.loadFromDisk(completion: { _ in
                WKWebsiteDataStore.default().httpCookieStore.getAllCookies({ cookies in
                  if cookies.contains(where: { $0.name == "BraveCookieTest" }) {
                    expectation.fulfill()
                  }
                })
              })
            })
        })
      }
    } else {
      XCTFail()
    }
    wait(for: [expectation], timeout: 30.0)
  }
}
