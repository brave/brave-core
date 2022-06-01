// Copyright 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import XCTest
import CryptoKit
@testable import Brave

class RandomConfigurationTests: XCTestCase {
  func testSameResultsForSameETLDAndSessionKey() throws {
    // Given
    // Different session keys and same eTLD+1
    let sessionKey = SymmetricKey(size: .bits256)
    let firstRandomConfiguration = RandomConfiguration(etld: "example.com", sessionKey: sessionKey)
    let secondRandomConfiguration = RandomConfiguration(etld: "example.com", sessionKey: sessionKey)

    // Then
    // Everything should equal
    XCTAssertEqual(
      firstRandomConfiguration.domainKey,
      secondRandomConfiguration.domainKey
    )

    XCTAssertEqual(
      firstRandomConfiguration.domainSignedKey(for: "TEST"),
      secondRandomConfiguration.domainSignedKey(for: "TEST")
    )

    // Except
    // When signing different strings
    XCTAssertNotEqual(
      firstRandomConfiguration.domainSignedKey(for: "TEST1"),
      secondRandomConfiguration.domainSignedKey(for: "TEST2")
    )
  }

  func testDifferentResultsForDifferentETLDAndSameSessionKey() throws {
    // Given
    // Same session keys but different eTLD+1
    let sessionKey = SymmetricKey(size: .bits256)
    let firstRandomConfiguration = RandomConfiguration(etld: "example.com", sessionKey: sessionKey)
    let secondRandomConfiguration = RandomConfiguration(etld: "brave.com", sessionKey: sessionKey)

    // Then
    // Nothing should equal
    XCTAssertNotEqual(
      firstRandomConfiguration.domainKey,
      secondRandomConfiguration.domainKey
    )

    XCTAssertNotEqual(
      firstRandomConfiguration.domainSignedKey(for: "TEST"),
      secondRandomConfiguration.domainSignedKey(for: "TEST")
    )

    XCTAssertNotEqual(
      firstRandomConfiguration.domainSignedKey(for: "TEST1"),
      secondRandomConfiguration.domainSignedKey(for: "TEST2")
    )
  }

  func testDifferentResultsForSameETLDAndDifferentSessionKey() throws {
    // Given
    // Different session keys but same eTLD+1
    let firstRandomConfiguration = RandomConfiguration(etld: "example.com", sessionKey: SymmetricKey(size: .bits256))
    let secondRandomConfiguration = RandomConfiguration(etld: "example.com", sessionKey: SymmetricKey(size: .bits256))

    // Then
    // Nothing should equal
    XCTAssertNotEqual(
      firstRandomConfiguration.domainKey,
      secondRandomConfiguration.domainKey
    )

    XCTAssertNotEqual(
      firstRandomConfiguration.domainSignedKey(for: "TEST"),
      secondRandomConfiguration.domainSignedKey(for: "TEST")
    )

    XCTAssertNotEqual(
      firstRandomConfiguration.domainSignedKey(for: "TEST1"),
      secondRandomConfiguration.domainSignedKey(for: "TEST2")
    )
  }

  func testByteSizeOfDomainKeyDataIs256Bytes() throws {
    // Given
    // A random configuration
    let randomConfiguration = RandomConfiguration(etld: "example.com", sessionKey: SymmetricKey(size: .bits256))

    // Then
    // Nothing should equal
    XCTAssertNotEqual(randomConfiguration.domainKeyData.getBytes().count, 256)
  }
}
