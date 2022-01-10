// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import XCTest
import BraveShared
@testable import Data
@testable import Client

class SafeBrowsingTests: XCTestCase {

    override func setUp() {
        super.setUp()
        
        Preferences.Shields.blockPhishingAndMalware.reset()
        DataController.shared = DataController()
        DataController.shared.initializeOnce()
    }

    override func tearDown() {
        DataController.viewContext.reset()
        super.tearDown()
    }

    func testShouldBlock() {
        XCTAssert(SafeBrowsing.isSafeBrowsingEnabledForURL(URL(string: "http://brave.com")!))
        XCTAssert(SafeBrowsing.isSafeBrowsingEnabledForURL(URL(string: "https://brave.com")!))
        XCTAssert(SafeBrowsing.isSafeBrowsingEnabledForURL(URL(string: "https://www.brave.com")!))
        XCTAssert(SafeBrowsing.isSafeBrowsingEnabledForURL(URL(string: "https://example.com")!))
    }
    
    func testShouldBlockGlobalShieldOff() {
        Preferences.Shields.blockPhishingAndMalware.value = false
        XCTAssertFalse(SafeBrowsing.isSafeBrowsingEnabledForURL(URL(string: "https://brave.com")!))
        XCTAssertFalse(SafeBrowsing.isSafeBrowsingEnabledForURL(URL(string: "https://example.com")!))
        XCTAssertFalse(SafeBrowsing.isSafeBrowsingEnabledForURL(URL(string: "https://foo.com")!))
    }
    
    func testShouldBlockLocalShields() {
        let context = DataController.viewContext
        let braveUrl = URL(string: "https://brave.com")!
        let exampleUrl = URL(string: "https://example.com")!
        
        // Brave domain will have safe browsing shield turned off.
        // No need to call getOrCreateForUrl here.
        Domain.setBraveShieldInternal(forUrl: braveUrl, shield: .SafeBrowsing, isOn: false, context: .existing(context))
        
        // example.com will have default value nil which means true
        _ = Domain.getOrCreate(forUrl: exampleUrl, persistent: true)
        
        // Global shield on, local shield should have precedence over global shield
        XCTAssertFalse(SafeBrowsing.isSafeBrowsingEnabledForURL(braveUrl))
        XCTAssert(SafeBrowsing.isSafeBrowsingEnabledForURL(exampleUrl))
        
        Preferences.Shields.blockPhishingAndMalware.value = false
        Domain.setBraveShieldInternal(forUrl: exampleUrl, shield: .SafeBrowsing, isOn: true, context: .existing(context))
        
        XCTAssertFalse(SafeBrowsing.isSafeBrowsingEnabledForURL(braveUrl))
        XCTAssert(SafeBrowsing.isSafeBrowsingEnabledForURL(exampleUrl))
        
    }
}
