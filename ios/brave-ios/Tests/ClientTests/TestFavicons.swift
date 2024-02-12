/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

import Foundation
import XCTest
import Storage
@testable import Brave
import Shared
import Data
import Favicon

@MainActor class TestFavicons: ProfileTest {
  
  override func setUp() {
    super.setUp()
    
    DataController.shared.initializeOnce()
  }

  func testBundledFavicons() async throws {
    let favicon = try await BundledFaviconRenderer.loadIcon(url: URL(string: "http://www.google.de")!)
    XCTAssertNotNil(favicon.image)
    let favicon2 = try await BundledFaviconRenderer.loadIcon(url: URL(string: "http://vancouver.craigslist.ca")!)
    XCTAssertNotNil(favicon2.image)
  }

  func testImageViewLoad() {
    let expectation = XCTestExpectation(description: "favicon.load")
    let imageView = UIImageView()
    imageView.loadFavicon(for: URL(string: "http://www.google.de")!, isPrivateBrowsing: false, monogramFallbackCharacter: nil) { _ in 
      // Should be a default icon therefore not truly async
      XCTAssertNotNil(imageView.image)
      expectation.fulfill()
    }
    wait(for: [expectation], timeout: 5.0)
  }

  func testIconTransparentAroundEdges() {
    let expectation = XCTestExpectation(description: "favicon.transparent.1")
    var image: UIImage!
    let size = CGSize(width: 48, height: 48)
    UIGraphicsBeginImageContext(size)
    let context = UIGraphicsGetCurrentContext()!
    context.setFillColor(UIColor.black.cgColor)
    context.addEllipse(in: CGRect(size: size).insetBy(dx: 5, dy: 5))
    context.fillPath()
    image = UIGraphicsGetImageFromCurrentImageContext()
    UIGraphicsEndImageContext()
    DispatchQueue.main.async {
      XCTAssert(image.hasTransparentEdges)
      expectation.fulfill()
    }
    wait(for: [expectation], timeout: 5.0)
  }

  func testIconNotTransparentAroundEdges() {
    let expectation = XCTestExpectation(description: "favicon.transparent.2")
    var image: UIImage!
    let size = CGSize(width: 48, height: 48)
    UIGraphicsBeginImageContext(size)
    let context = UIGraphicsGetCurrentContext()!
    context.setFillColor(UIColor.black.cgColor)
    context.addRect(CGRect(size: size))
    context.fillPath()
    image = UIGraphicsGetImageFromCurrentImageContext()
    UIGraphicsEndImageContext()
    DispatchQueue.main.async {
      XCTAssert(image.hasTransparentEdges)
      expectation.fulfill()
    }
    wait(for: [expectation], timeout: 15.0)
  }

  func testIconTransparencyDoesntCrashWithEmptyImage() {
    let expectation = XCTestExpectation(description: "favicon.transparent.3")
    let image = UIImage()
    DispatchQueue.main.async {
      XCTAssert(image.hasTransparentEdges)
      expectation.fulfill()
    }
    wait(for: [expectation], timeout: 5.0)
  }
}
