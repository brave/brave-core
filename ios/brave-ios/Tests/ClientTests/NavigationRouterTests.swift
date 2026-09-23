// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import Foundation
import Preferences
import WebKit
import XCTest

@testable import Brave

class NavigationRouterTests: XCTestCase {

  private let privateBrowsingManager = PrivateBrowsingManager()

  override func setUp() {
    super.setUp()
    privateBrowsingManager.isPrivateBrowsing = false
    Preferences.Privacy.privateBrowsingOnly.reset()
  }

  override func tearDown() {
    super.tearDown()
    privateBrowsingManager.isPrivateBrowsing = false
    Preferences.Privacy.privateBrowsingOnly.reset()
  }

  var appScheme: String {
    "brave"
  }

  func testOpenURLScheme() {
    let testURLEncoding = { [appScheme] (url: String) -> Bool in
      guard let escaped = url.escape() else {
        XCTFail("URL Cannot be escaped")
        return false
      }

      guard let appURL = URL(string: "\(appScheme)://open-url?url=\(escaped)") else {
        XCTFail("Application URL is Invalid")
        return false
      }

      guard
        let navItem = NavigationPath(
          url: appURL,
          isPrivateBrowsing: self.privateBrowsingManager.isPrivateBrowsing
        )
      else {
        XCTFail("Invalid Navigation Path")
        return false
      }

      return navItem == NavigationPath.url(webURL: URL(string: url)!, isPrivate: false)
    }

    // Test regular URL with no escape characters
    XCTAssertTrue(testURLEncoding("http://google.com?a=1&b=2"))

    // Test URL with single escape encoded characters
    XCTAssertTrue(testURLEncoding("http://google.com?a=1&b=2&c=foo%20bar"))

    // Test URL with double escaped encoded characters (URL was encoded twice)
    XCTAssertTrue(testURLEncoding("http://google.com%3Fa%3D1%26b%3D2%26c%3Dfoo%2520bar"))

    let emptyNav = NavigationPath(
      url: URL(string: "\(appScheme)://open-url?private=true")!,
      isPrivateBrowsing: privateBrowsingManager.isPrivateBrowsing
    )
    XCTAssertEqual(emptyNav, NavigationPath.url(webURL: nil, isPrivate: true))

    let badNav = NavigationPath(
      url: URL(string: "\(appScheme)://open-url?url=blah")!,
      isPrivateBrowsing: privateBrowsingManager.isPrivateBrowsing
    )
    XCTAssertEqual(badNav, NavigationPath.url(webURL: URL(string: "blah"), isPrivate: false))
  }

  func testSearchScheme() {
    let query = "Foo Bar".addingPercentEncoding(withAllowedCharacters: .alphanumerics)!
    let appURL = "\(appScheme)://search?q=" + query
    let navItem = NavigationPath(
      url: URL(string: appURL)!,
      isPrivateBrowsing: privateBrowsingManager.isPrivateBrowsing
    )!

    XCTAssertEqual(navItem, NavigationPath.text("Foo Bar"))
  }

  func testDefaultNavigationPath() {
    let url = URL(string: "https://duckduckgo.com")!
    let appURL = URL(string: "\(self.appScheme)://open-url?url=\(url.absoluteString.escape()!)")!
    let path = NavigationPath(
      url: appURL,
      isPrivateBrowsing: privateBrowsingManager.isPrivateBrowsing
    )!

    XCTAssertEqual(path, NavigationPath.url(webURL: url, isPrivate: false))
  }

  func testNavigationPathInPrivateBrowsingOnlyMode() {
    Preferences.Privacy.privateBrowsingOnly.value = true

    let url = URL(string: "https://duckduckgo.com")!
    let appURL = URL(string: "\(self.appScheme)://open-url?url=\(url.absoluteString.escape()!)")!
    let path = NavigationPath(
      url: appURL,
      isPrivateBrowsing: privateBrowsingManager.isPrivateBrowsing
    )!

    // Should inheritely be private by default
    XCTAssertEqual(path, NavigationPath.url(webURL: url, isPrivate: true))
  }

  func testNavigationPathAlreadyInPrivateBrowsingMode() {
    privateBrowsingManager.isPrivateBrowsing = true

    let url = URL(string: "https://duckduckgo.com")!
    let appURL = URL(string: "\(self.appScheme)://open-url?url=\(url.absoluteString.escape()!)")!
    let path = NavigationPath(
      url: appURL,
      isPrivateBrowsing: privateBrowsingManager.isPrivateBrowsing
    )!

    // Should inheritely be private by default
    XCTAssertEqual(path, NavigationPath.url(webURL: url, isPrivate: true))
  }

  func testNavigationPathForcedRegularMode() {
    privateBrowsingManager.isPrivateBrowsing = true

    let url = URL(string: "https://duckduckgo.com")!
    let appURL = URL(
      string: "\(self.appScheme)://open-url?url=\(url.absoluteString.escape()!)&private=false"
    )!
    let path = NavigationPath(
      url: appURL,
      isPrivateBrowsing: privateBrowsingManager.isPrivateBrowsing
    )!

    // Should be regular due to specified argument in the URL
    XCTAssertEqual(path, NavigationPath.url(webURL: url, isPrivate: false))
  }
}
