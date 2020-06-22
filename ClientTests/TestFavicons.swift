/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

import Foundation
import XCTest
import Storage
@testable import Client
import Shared

class TestFavicons: ProfileTest {
    
    func testBundledFavicons() {
        let fetcher = FaviconFetcher(siteURL: URL(string: "http://www.google.de")!, kind: .favicon)
        XCTAssertNotNil(fetcher.bundledIcon)
        let fetcher2 = FaviconFetcher(siteURL: URL(string: "http://vancouver.craigslist.ca")!, kind: .favicon)
        XCTAssertNotNil(fetcher2.bundledIcon)
    }
    
    func testImageViewLoad() {
        let expectation = XCTestExpectation(description: "favicon.load")
        let imageView = UIImageView()
        imageView.loadFavicon(for: URL(string: "http://www.google.de")!) {
            // Should be a default icon therefore not truly async
            XCTAssertNotNil(imageView.image)
            expectation.fulfill()
        }
        wait(for: [expectation], timeout: 5.0)
    }
}
