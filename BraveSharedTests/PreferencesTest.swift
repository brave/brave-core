// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import XCTest
import BraveShared

private let optionalStringDefault: String? = nil
private let intDefault: Int = 1

extension Preferences {
    // Test preferences
    fileprivate static let optionalStringOption = Option<String?>(key: "option-one", default: optionalStringDefault)
    fileprivate static let intOption = Option<Int>(key: "option-two", default: intDefault)
}

class PreferencesTest: XCTestCase {

    override func setUp() {
        Preferences.optionalStringOption.reset()
        Preferences.intOption.reset()
        
        XCTAssertEqual(optionalStringDefault, Preferences.optionalStringOption.value)
        XCTAssertEqual(intDefault, Preferences.intOption.value)
    }

    func testSettingPreference() {
        let newString = "test"
        let optionalStringOption = Preferences.optionalStringOption
        optionalStringOption.value = newString
        XCTAssertEqual(newString, optionalStringOption.value)
        XCTAssertEqual(newString, optionalStringOption.container.string(forKey: optionalStringOption.key))
        
        let newInt = 2
        let intOption = Preferences.intOption
        intOption.value = newInt
        XCTAssertEqual(newInt, intOption.value)
        XCTAssertEqual(newInt, intOption.container.integer(forKey: Preferences.intOption.key))
    }

    func testResetPreference() {
        Preferences.optionalStringOption.value = "test"
        Preferences.intOption.value = 2
        XCTAssertEqual("test", Preferences.optionalStringOption.value)
        XCTAssertEqual(2, Preferences.intOption.value)
        
        Preferences.optionalStringOption.reset()
        XCTAssertEqual(optionalStringDefault, Preferences.optionalStringOption.value)
        XCTAssertNil(Preferences.optionalStringOption.container.string(forKey: Preferences.optionalStringOption.key))
        Preferences.intOption.reset()
        XCTAssertEqual(intDefault, Preferences.intOption.value)
    }

}
