// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import Preferences
import Shared
import WebKit
import XCTest

@testable import Brave
@testable import UserAgent

class UserAgentTests: XCTestCase {

  override func setUp() {
    super.setUp()
    Preferences.UserAgent.alwaysRequestDesktopSite.reset()
  }

  let desktopUARegex: (String) -> Bool = { ua in
    let range = ua.range(
      of:
        "^Mozilla/5\\.0 \\(Macintosh; Intel Mac OS X [0-9_]+\\) AppleWebKit/[0-9\\.]+ \\(KHTML, like Gecko\\) Version/[0-9\\.]+ Safari/[0-9\\.]+$",
      options: .regularExpression
    )
    return range != nil
  }

  let mobileUARegex: (String) -> Bool = { ua in
    let cpuPart =
      UIDevice.isPhone
      ? "\\(iPhone; CPU iPhone OS [0-9_]+ like Mac OS X\\)"
      : "\\(iPad; CPU OS [0-9_]+ like Mac OS X\\)"

    let range = ua.range(
      of:
        "^Mozilla/5\\.0 \(cpuPart) AppleWebKit/[0-9\\.]+ \\(KHTML, like Gecko\\) Version/[0-9\\.]+ Mobile/[A-Za-z0-9]+ Safari/[0-9\\.]+$",
      options: .regularExpression
    )
    return range != nil
  }

  // Simple test to make sure the WKWebView UA matches the expected FxiOS pattern.
  func testBraveWebViewUserAgentOnPhone() {
    if UIDevice.current.userInterfaceIdiom != .phone { return }

    XCTAssertTrue(mobileUARegex(UserAgent.mobile), "User agent computes correctly.")

    let expectation = self.expectation(description: "Found Firefox user agent")

    let webView = BraveWebView(frame: .zero, isPrivate: false)

    webView.evaluateSafeJavaScript(
      functionName: "navigator.userAgent",
      contentWorld: .page,
      asFunction: false
    ) { result, error in
      let userAgent = result as! String
      if !self.mobileUARegex(userAgent) || self.desktopUARegex(userAgent) {
        XCTFail("User agent did not match expected pattern! \(userAgent)")
      }
      expectation.fulfill()
    }

    waitForExpectations(timeout: 60, handler: nil)
  }

  // WKWebView doesn't give us all UA parts of Safari
  // we are able to compare only the first part.
  func testFirstUAPart() {
    let expectation = self.expectation(description: "First part of UA comparison")

    let webView = BraveWebView(frame: .zero, isPrivate: false)
    let wkWebView = WKWebView()

    webView.evaluateSafeJavaScript(
      functionName: "navigator.userAgent",
      args: [],
      contentWorld: .page,
      asFunction: false
    ) { result, error in

      guard let braveFirstPartOfUA = (result as? String)?.components(separatedBy: "Gecko") else {
        XCTFail("Could not unwrap BraveWebView UA")
        return
      }

      wkWebView.evaluateSafeJavaScript(
        functionName: "navigator.userAgent",
        contentWorld: .page,
        asFunction: false
      ) { wkResult, wkError in
        guard
          let wkWebViewFirstPartOfUA = (result as? String)?
            .components(separatedBy: "Gecko")
        else {
          XCTFail("Could not unwrap WKWebView UA")
          return
        }

        if braveFirstPartOfUA == wkWebViewFirstPartOfUA {
          expectation.fulfill()
        } else {
          XCTFail("BraveWebView and WKWebView user agents do not match.")
        }
      }
    }

    waitForExpectations(timeout: 60, handler: nil)
  }

  func testDesktopUserAgentOnPad() {
    Preferences.UserAgent.alwaysRequestDesktopSite.value = true

    XCTAssertTrue(desktopUARegex(UserAgent.desktop), "User agent computes correctly.")

    let userAgent = UserAgent.userAgentForIdiom(.pad)

    if self.mobileUARegex(userAgent) || !self.desktopUARegex(userAgent) {
      XCTFail("User agent did not match expected pattern! \(userAgent)")
    }
  }

  func testMobileUserAgentOnPad() {
    Preferences.UserAgent.alwaysRequestDesktopSite.value = false

    XCTAssertTrue(mobileUARegex(UserAgent.mobile), "User agent computes correctly.")
    let userAgent = UserAgent.userAgentForIdiom(.pad)

    if !self.mobileUARegex(userAgent) || self.desktopUARegex(userAgent) {
      XCTFail("User agent did not match expected pattern! \(userAgent)")
    }
  }
}
