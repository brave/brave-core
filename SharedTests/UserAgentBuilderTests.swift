// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import XCTest
import WebKit
import Shared

class UserAgentBuilderTests: XCTestCase {

    func testUserAgentBuilder() {
        let builderUA = UserAgentBuilder().build()
        
        var wkWebViewUA = ""
        
        let webViewUAExpectation = expectation(description: "WKWebView user agent")
        let webView = WKWebView(frame: CGRect(x: 0, y: 0, width: 100, height: 100))
        
        webView.evaluateJavaScript("navigator.userAgent") { ua, error in
            wkWebViewUA = ua as! String
            webViewUAExpectation.fulfill()
        }
        
        wait(for: [webViewUAExpectation], timeout: 10)
        
        XCTAssertEqual(builderUA, wkWebViewUA)
        
    }

}
