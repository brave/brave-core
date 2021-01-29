// Copyright 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import XCTest
import Shared
import BraveShared
import WebKit
@testable import Client

class WKWebViewExtensionsTest: XCTestCase {
    func testGenerateJavascriptFunctionString() {
        let webView = BraveWebView(frame: .zero, isPrivate: false)
        var js = webView.generateJavascriptFunctionString(functionName: "demo_function", args: [])
        XCTAssertNil(js.error)
        XCTAssertEqual(js.javascript, "demo_function()")
        
        js = webView.generateJavascriptFunctionString(functionName: "demo_function", args: ["a", "b", "c"])
        XCTAssertNil(js.error)
        XCTAssertEqual(js.javascript, "demo_function('a', 'b', 'c')")
        
        js = webView.generateJavascriptFunctionString(functionName: "demo_function", args: ["\"); (fn () {userPassword = 7})("])
        XCTAssertNotNil(js.error)
        
        js = webView.generateJavascriptFunctionString(functionName: "demo_function", args: ["&", "'", "<", ">", "`"])
        XCTAssertNotNil(js.error)

        js = webView.generateJavascriptFunctionString(functionName: "demo_function", args: ["<script>alert(1);</script>"])
        XCTAssertNil(js.error)
        XCTAssertEqual(js.javascript, "demo_function('&lt;script&gt;alert(1);&lt;/script&gt;')")
        
        js = webView.generateJavascriptFunctionString(functionName: "demo_function", args: ["<script>alert(1);</script>"], escapeArgs: false)
        XCTAssertNil(js.error)
        XCTAssertEqual(js.javascript, "demo_function(<script>alert(1);</script>)")
    }
}
