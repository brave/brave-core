// Copyright 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import XCTest
@testable import Client

class UserScriptManagerTest: XCTestCase {
    func testisMessageHandlerTokenMissing() {
        var body: [String: Any] = [
            "data": 1,
            "securitytoken": UserScriptManager.messageHandlerToken.uuidString
        ]
        XCTAssertFalse(UserScriptManager.isMessageHandlerTokenMissing(in: body))

        body = [
            "data": 1,
            "securitytoken": "test"
        ]
        XCTAssertTrue(UserScriptManager.isMessageHandlerTokenMissing(in: body))

        body = [
            "data": 1,
        ]
        XCTAssertTrue(UserScriptManager.isMessageHandlerTokenMissing(in: body))
    }
}
