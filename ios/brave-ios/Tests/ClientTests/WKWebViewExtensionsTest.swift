// Copyright 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveShared
import Shared
import WebKit
import XCTest

@testable import Brave

class WKWebViewExtensionsTest: XCTestCase {
  func testGenerateJavascriptFunctionString() {
    let webView = WKWebView(frame: .zero)
    var js = webView.generateJSFunctionString(functionName: "demo_function", args: [])
    XCTAssertNil(js.error)
    XCTAssertEqual(js.javascript, "demo_function()")

    js = webView.generateJSFunctionString(functionName: "demo_function", args: ["a", "b", "c"])
    XCTAssertNil(js.error)
    XCTAssertEqual(js.javascript, "demo_function('a', 'b', 'c')")

    js = webView.generateJSFunctionString(
      functionName: "demo_function",
      args: ["\"); (fn () {userPassword = 7})("]
    )
    XCTAssertNil(js.error)

    js = webView.generateJSFunctionString(
      functionName: "demo_function",
      args: ["&", "'", "<", ">", "`"]
    )
    XCTAssertNil(js.error)

    js = webView.generateJSFunctionString(
      functionName: "demo_function",
      args: ["<script>alert(1);</script>"]
    )
    XCTAssertNil(js.error)
    XCTAssertEqual(js.javascript, "demo_function('&lt;script&gt;alert(1);&lt;/script&gt;')")

    js = webView.generateJSFunctionString(
      functionName: "demo_function",
      args: ["<script>alert(1);</script>"],
      escapeArgs: false
    )
    XCTAssertNil(js.error)
    XCTAssertEqual(js.javascript, "demo_function(<script>alert(1);</script>)")
  }
}
