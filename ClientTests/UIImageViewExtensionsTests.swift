/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

import UIKit
import XCTest
import Storage
import SDWebImage
import GCDWebServers
import Shared

@testable import Client

class UIImageViewExtensionsTests: XCTestCase {

    override func setUp() {
        SDWebImageDownloader.shared().urlCredential = WebServer.sharedInstance.credentials
    }

    func testAsyncSetIcon() {
        let originalImage = #imageLiteral(resourceName: "fxLogo")

        WebServer.sharedInstance.registerHandlerForMethod("GET", module: "favicon", resource: "icon") { (request) -> GCDWebServerResponse? in
            return GCDWebServerDataResponse(data: originalImage.pngData()!, contentType: "image/png")
        }

        let favImageView = UIImageView()
        favImageView.setIcon(Favicon(url: "http://localhost:6571/favicon/icon"), forURL: URL(string: "http://localhost:6571"))

        let expect = expectation(description: "UIImageView async load")
        let time = Int64(2 * Double(NSEC_PER_SEC))
        DispatchQueue.main.asyncAfter(deadline: DispatchTime.now() + Double(time) / Double(NSEC_PER_SEC)) {
            XCTAssert(originalImage.isStrictlyEqual(to: favImageView.image!), "The correct favicon should be applied to the UIImageView")
            expect.fulfill()
        }
        waitForExpectations(timeout: 5, handler: nil)
    }

    func testAsyncSetIconFail() {
        let favImageView = UIImageView()

        let gFavURL = URL(string: "https://www.nofavicon.com/noicon.ico")
        let gURL = URL(string: "http://nofavicon.com")
        let correctImage = FaviconFetcher.getDefaultFavicon(gURL!)

        favImageView.setIcon(Favicon(url: gFavURL!.absoluteString), forURL: gURL)

        let expect = expectation(description: "UIImageView async load")
        let time = Int64(2 * Double(NSEC_PER_SEC))
        DispatchQueue.main.asyncAfter(deadline: DispatchTime.now() + Double(time) / Double(NSEC_PER_SEC)) {
           XCTAssert(correctImage.isStrictlyEqual(to: favImageView.image!), "The correct default favicon should be applied to the UIImageView")
            expect.fulfill()
        }
        waitForExpectations(timeout: 5, handler: nil)
    }
}
