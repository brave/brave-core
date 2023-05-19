// Copyright 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import XCTest
import CryptoKit
@testable import Brave

@MainActor class FarblingProtectionHelperTests: XCTestCase {
  func testGivenTheSameRandomManagerThenSameFakePluginData() throws {
    // Given
    // Same random manager
    let sessionKey = SymmetricKey(size: .bits256)
    let randomConfiguration = RandomConfiguration(etld: "example.com", sessionKey: sessionKey)

    // Then
    // Same results
    XCTAssertEqual(
      try FarblingProtectionHelper.makeFarblingParams(from: randomConfiguration),
      try FarblingProtectionHelper.makeFarblingParams(from: randomConfiguration)
    )
  }

  func testGivenDifferentRandomManagerThenDifferentFakePluginData() throws {
    // Given
    // Different random manager
    let sessionKey = SymmetricKey(size: .bits256)
    let firstRandomConfiguration = RandomConfiguration(etld: "example.com", sessionKey: sessionKey)
    let secondRandomConfiguration = RandomConfiguration(etld: "brave.com", sessionKey: sessionKey)

    // Then
    // Different results
    XCTAssertNotEqual(
      try FarblingProtectionHelper.makeFarblingParams(from: firstRandomConfiguration),
      try FarblingProtectionHelper.makeFarblingParams(from: secondRandomConfiguration)
    )
  }
}
