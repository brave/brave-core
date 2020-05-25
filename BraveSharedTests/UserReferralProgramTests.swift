// Copyright 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import XCTest
@testable import BraveShared


class UserReferralProgramTests: XCTestCase {

    private static let clipboardPrefix = "F83AB73F-9852-4F01-ABA8-7830B8825993"

    func testSanitzerGoodInput() throws {
        let response = UserReferralProgram.sanitize(input: "\(UserReferralProgramTests.clipboardPrefix):ABC123")
        XCTAssertNotNil(response)
    }
    
    func testSanitzerBadPrefix() throws {
        let response = UserReferralProgram.sanitize(input: "BADPREFIX:ABC123")
        XCTAssertNil(response)
    }
    
    func testSanitzerBadCode() throws {
        var response = UserReferralProgram.sanitize(input: "\(UserReferralProgramTests.clipboardPrefix):ABCA123")
        XCTAssertNil(response)
        
        response = UserReferralProgram.sanitize(input: "\(UserReferralProgramTests.clipboardPrefix):ABc123")
        XCTAssertNil(response)

        response = UserReferralProgram.sanitize(input: "\(UserReferralProgramTests.clipboardPrefix):ABC1234")
        XCTAssertNil(response)
        
        response = UserReferralProgram.sanitize(input: "\(UserReferralProgramTests.clipboardPrefix):ABC12#")
        XCTAssertNil(response)
        
        response = UserReferralProgram.sanitize(input: "\(UserReferralProgramTests.clipboardPrefix):ABC12")
        XCTAssertNil(response)
        
        response = UserReferralProgram.sanitize(input: "\(UserReferralProgramTests.clipboardPrefix):AB123")
        XCTAssertNil(response)
        
        response = UserReferralProgram.sanitize(input: "\(UserReferralProgramTests.clipboardPrefix):ÀBC123")
        XCTAssertNil(response)

    }
    
    func testSanitzerNoInput() throws {
        let response = UserReferralProgram.sanitize(input: nil)
        XCTAssertNil(response)
    }
    
    func testSanitzerEmptyInput() throws {
        var response = UserReferralProgram.sanitize(input: "")
        XCTAssertNil(response)
        
        response = UserReferralProgram.sanitize(input: "\(UserReferralProgramTests.clipboardPrefix):")
        XCTAssertNil(response)
        
        response = UserReferralProgram.sanitize(input: "\(UserReferralProgramTests.clipboardPrefix)")
        XCTAssertNil(response)
    }
    
    func testNoSpacer() throws {
        let response = UserReferralProgram.sanitize(input: "\(UserReferralProgramTests.clipboardPrefix)ABC123")
        XCTAssertNil(response)
    }
}
