// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import XCTest
import Preferences

private let optionalStringDefault: String? = nil
private let intDefault: Int = 1
private let optionalStringEnumDefault: StringEnum? = nil
private let stringEnumDefault: StringEnum = .a
private let intEnumDefault: IntEnum = .one

extension Preferences {
  // Test preferences
  fileprivate static let optionalStringOption = Option<String?>(key: "option-one", default: optionalStringDefault)
  fileprivate static let intOption = Option<Int>(key: "option-two", default: intDefault)
  fileprivate static let stringEnumOption = Option<StringEnum>(key: "option-three", default: stringEnumDefault)
  fileprivate static let intEnumOption = Option<IntEnum>(key: "option-four", default: intEnumDefault)
  fileprivate static let optionalStringEnumOption = Option<StringEnum?>(key: "option-five", default: optionalStringEnumDefault)
}

private enum StringEnum: String {
  case a, b, c
}
private enum IntEnum: Int {
  case one = 1, two = 2, three = 3
}

class PreferencesTest: XCTestCase {

  override func setUp() {
    Preferences.optionalStringOption.reset()
    Preferences.intOption.reset()
    Preferences.stringEnumOption.reset()
    Preferences.optionalStringOption.reset()
    Preferences.intEnumOption.reset()

    XCTAssertEqual(optionalStringDefault, Preferences.optionalStringOption.value)
    XCTAssertEqual(intDefault, Preferences.intOption.value)
    XCTAssertEqual(stringEnumDefault, Preferences.stringEnumOption.value)
    XCTAssertEqual(optionalStringEnumDefault, Preferences.optionalStringEnumOption.value)
    XCTAssertEqual(intEnumDefault, Preferences.intEnumOption.value)
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
    
    optionalStringOption.value = nil
    XCTAssertEqual(optionalStringOption.value, nil)
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

  func testEnumPreference() {
    Preferences.stringEnumOption.value = .b
    Preferences.intEnumOption.value = .two
    
    XCTAssertEqual(Preferences.stringEnumOption.value, .b)
    XCTAssertEqual(Preferences.intEnumOption.value, .two)
    
    // Reset restoring an enum
    let stringEnumOption = Preferences.Option<StringEnum>(key: "option-three", default: stringEnumDefault)
    XCTAssertEqual(Preferences.stringEnumOption.value, stringEnumOption.value)
    
    let intEnumOption = Preferences.Option<IntEnum>(key: "option-four", default: intEnumDefault)
    XCTAssertEqual(Preferences.intEnumOption.value, intEnumOption.value)
    
    // Optional
    Preferences.optionalStringEnumOption.value = .a
    XCTAssertEqual(Preferences.optionalStringEnumOption.value, .a)
    
    let optionalStringEnumOption = Preferences.Option<StringEnum?>(key: "option-five", default: optionalStringEnumDefault)
    XCTAssertEqual(Preferences.optionalStringEnumOption.value, optionalStringEnumOption.value)
    
    Preferences.optionalStringEnumOption.value = nil
    XCTAssertEqual(Preferences.optionalStringEnumOption.value, nil)
  }
}
