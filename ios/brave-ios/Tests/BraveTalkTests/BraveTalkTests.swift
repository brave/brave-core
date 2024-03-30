// Copyright 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

import Foundation
@_spi(AppLaunch) import Shared
import XCTest

@testable import BraveTalk

class BraveTalkTests: XCTestCase {
  @MainActor func testBraveTalkJitsiIntegrationEnabledOnRelease() {
    AppConstants.setBuildChannel(.release)
    XCTAssertTrue(BraveTalkJitsiCoordinator.isIntegrationEnabled)
  }

  @MainActor func testBraveTalkJitsiIntegrationEnabledOnBeta() {
    AppConstants.setBuildChannel(.beta)
    XCTAssertTrue(BraveTalkJitsiCoordinator.isIntegrationEnabled)
  }
}
