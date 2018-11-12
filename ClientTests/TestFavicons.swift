/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

import Foundation
import XCTest
import Storage
@testable import Client
import Shared

class TestFavicons: ProfileTest {

    fileprivate func addSite(_ favicons: Favicons, url: String, s: Bool = true) {
        let expectation = self.expectation(description: "Wait for history")
        let site = Site(url: url, title: "")
        let icon = Favicon(url: url + "/icon.png")
        favicons.addFavicon(icon, forSite: site).upon {
            XCTAssertEqual($0.isSuccess, s, "Icon added \(url)")
            expectation.fulfill()
        }
        self.waitForExpectations(timeout: 100, handler: nil)
    }

    func testDefaultFavicons() {
        let icon = FaviconFetcher.getDefaultIconForURL(url: URL(string: "http://www.google.de")!)
        XCTAssertNotNil(icon)
        let craigsListIcon = FaviconFetcher.getDefaultIconForURL(url: URL(string: "http://vancouver.craigslist.ca")!)
        XCTAssertNotNil(craigsListIcon)

    }
}
