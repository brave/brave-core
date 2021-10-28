// Copyright 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import XCTest
@testable import BraveWallet

class PasswordValidationTests: XCTestCase {
    func testValidPasswords() {
        // At least one of each
        XCTAssertTrue(PasswordValidation.isValid("password1!"))
        // Multiple numbers and special characters
        XCTAssertTrue(PasswordValidation.isValid("4-mL2pv4dfgwV*xabY8LeQNeGJMsF-kt"))
        // All valid characters
        XCTAssertTrue(PasswordValidation.isValid(#"abcdefghijklmnopqrstuvwxyz0123456789`~!@#$%^&*()+=?;:|<>,.'"{}/[]\-_"#))
    }
    func testInvalidPasswords() {
        // Too short
        XCTAssertFalse(PasswordValidation.isValid("pass"))
        // No numbers or special characters
        XCTAssertFalse(PasswordValidation.isValid("password"))
        // No numbers
        XCTAssertFalse(PasswordValidation.isValid("password!"))
        // No special characters
        XCTAssertFalse(PasswordValidation.isValid("password1"))
        // Invalid special character
        XCTAssertFalse(PasswordValidation.isValid("password ®"))
    }
}
