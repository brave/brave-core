// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation
import XCTest
@testable import Client
import BraveShared
import Data
import WebKit

class FingerprintProtectionTest: XCTestCase {
    
    override func setUp() {
        DataController.shared = DataController()
    }
    
    func testFingerprintProtection() {
        // Let the app startup, prevents the tab from not being selected
        wait(2)
        
        let url = URL(string: "https://panopticlick.eff.org/results")!
        
        // Enable fingerprinting protection for the domain
        let domain = Domain.getOrCreateForUrl(url, context: DataController.viewContext)
        domain.shield_fpProtection = true as NSNumber
        DataController.save(context: DataController.viewContext)
        
        // Create the web page load delegate/expectation
        let navExpectation = expectation(description: "Navigation")
        let navDelegate = FingerprintProtectionNavDelegate(completed: { error in
            if let error = error {
                XCTFail("Failed to load the web page: \(error.localizedDescription)")
            }
            navExpectation.fulfill()
        })
        
        // Grab the tab manager
        let tabManager = (UIApplication.shared.delegate as! AppDelegate).browserViewController.tabManager
        tabManager.addNavigationDelegate(navDelegate)
        
        // Add the tab
        let tab = tabManager.addTabAndSelect(URLRequest(url: url))
        
        // Wait until the tabs fully loaded the page and give some buffer on completion
        wait(for: [navExpectation], timeout: 10.0)
        wait(3)
        
        // Expand the details
        tab.webView?.evaluateJavaScript("document.getElementById('showFingerprintLink2').click()", completionHandler: nil)
        
        wait(1)
        
        // Check the hash of the canvas fingerprint
        let innerHTMLCheck = expectation(description: "Hash of Canvas Fingerprint")
        tab.webView?.evaluateJavaScript("document.body.innerHTML", completionHandler: { (value, error) in
            defer { innerHTMLCheck.fulfill() }
            XCTAssertNil(error)
            guard let innerHTML = value as? String else {
                XCTFail("Incorrect type found")
                return
            }
            XCTAssert(innerHTML.contains("891f3debe00dbd3d1f0457a70d2f5213"))
        })
        
        let matchCheck = expectation(description: "Match Check")
        tab.webView?.evaluateJavaScript("/webgl fingerprint.*(\\n.+)*undetermined/gim.exec(document.body.innerHTML)[0]", completionHandler: { (value, error) in
            defer { matchCheck.fulfill() }
            XCTAssertNil(error)
            guard let match = value as? String else {
                XCTFail("Incorrect type found")
                return
            }
            XCTAssert(match.count > 50 && match.count < 300)
        })
        
        wait(for: [innerHTMLCheck, matchCheck], timeout: 10)
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
