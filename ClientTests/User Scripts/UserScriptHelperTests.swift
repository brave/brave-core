// Copyright 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import XCTest
import WebKit
@testable import Client

class UserScriptHelperTests: XCTestCase {
  func testGetDomainScriptTypes() throws {
    // Unfortuantely we can't customize the `WKNavigationAction` so we can't test anything here.
    // We would need to do other testing (perhaps UI testing? or a protocol of some sort?)
    // TODO: @JS Use some protocol instead of `WKNavigationAction` so we can pass a fake object?
  }
}
