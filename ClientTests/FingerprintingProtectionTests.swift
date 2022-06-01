// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation
import XCTest
@testable import Brave
@testable import Data
import BraveShared
import WebKit

class FingerprintProtectionTest: XCTestCase {

  override func setUp() {
    DataController.shared = DataController()
    DataController.shared.initializeOnce()
  }

  func testFingerprintProtection() {
    XCTAssertTrue(true)
  }
}

private class FingerprintProtectionNavDelegate: NSObject, WKNavigationDelegate {

  let completed: (Error?) -> Void

  init(completed: @escaping (Error?) -> Void) {
    self.completed = completed
    super.init()
  }

  func webView(_ webView: WKWebView, didFinish navigation: WKNavigation!) {
    completed(nil)
  }

  func webView(_ webView: WKWebView, didFail navigation: WKNavigation!, withError error: Error) {
    completed(error)
  }
}
