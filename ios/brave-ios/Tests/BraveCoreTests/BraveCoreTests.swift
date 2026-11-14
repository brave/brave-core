// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import BraveCore
import XCTest

final class BraveCoreTests: XCTestCase {
  @MainActor func testBraveCoreSetupAndTearDown() async {
    let main = BraveCoreMain()
    let profile = await main.loadDefaultProfile()
    XCTAssertFalse(profile.profile.name.isEmpty)
  }
}
