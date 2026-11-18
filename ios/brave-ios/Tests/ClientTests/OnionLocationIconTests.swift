// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import DesignSystem
import UIKit
import XCTest

@MainActor
final class OnionLocationIconTests: XCTestCase {
  func testOnionLocationIconIsIncludedInIOSAssetBundle() throws {
    let image = try XCTUnwrap(UIImage(braveSystemNamed: "leo.product.tor"))
    XCTAssertGreaterThan(image.size.width, 0)
    XCTAssertGreaterThan(image.size.height, 0)
  }
}
