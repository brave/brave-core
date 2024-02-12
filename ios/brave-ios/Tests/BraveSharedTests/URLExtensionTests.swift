// Copyright 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation
import XCTest

class URLExtensionTests: XCTestCase {
  func testOrigin() {
    let urls = [
      ("https://www.example.com/index.html", "https://www.example.com"),
      ("https://user:pass@m.foo.com/bar/baz?noo=abc#123", "https://m.foo.com"),
      ("http://blabla.com/test@test", "http://blabla.com"),
      ("http://test.com:8080", "http://test.com:8080"),
      ("http://test:test@[2606:4700:20::681a:a72]:8080/", "http://[2606:4700:20::681a:a72]:8080"),
    ]
    
    let badurls = [
      "data://google.com",
      "url:http://blabla.com/test:test",
      "http://test:t/est",
    ]
    
    urls.forEach { XCTAssertEqual(URL(string: $0.0)?.origin.serialized, $0.1) }
    badurls.forEach { XCTAssertTrue(URL(string: $0)?.origin.isOpaque ?? true) }
  }
  
  func testStrippedInternalURL() {
    let urls = [
      ("internal://local/web3/ddns?service_id=ethereum&url=http%3A%2F%2Fvitalik%2Eeth%2F", URL(string: "http://vitalik.eth/")),
      ("internal://local/sessionrestore?url=https://en.m.wikipedia.org/wiki/Main_Page", URL(string: "https://en.m.wikipedia.org/wiki/Main_Page")),
      ("internal://local/reader-mode?url=https://en.m.wikipedia.org/wiki/Main_Page", URL(string: "https://en.m.wikipedia.org/wiki/Main_Page"))
    ]
    
    urls.forEach { XCTAssertEqual(URL(string: $0.0)!.stippedInternalURL, $0.1) }
  }

}
