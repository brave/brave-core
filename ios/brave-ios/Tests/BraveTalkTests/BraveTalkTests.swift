// Copyright 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import Foundation
@testable import BraveTalk
import Shared
import XCTest

class BraveTalkTests: XCTestCase {
  @MainActor func testBraveTalkJitsiIntegrationEnabledOnRelease() {
    AppConstants.buildChannel = .release
    XCTAssertTrue(BraveTalkJitsiCoordinator.isIntegrationEnabled)
  }
  
  @MainActor func testBraveTalkJitsiIntegrationEnabledOnBeta() {
    AppConstants.buildChannel = .beta
    XCTAssertTrue(BraveTalkJitsiCoordinator.isIntegrationEnabled)
  }
}
